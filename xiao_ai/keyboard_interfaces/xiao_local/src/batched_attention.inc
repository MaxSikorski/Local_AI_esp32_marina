bool rf_attention0(
    std::uint32_t layer,
    std::size_t position,
    bool print_reference_stats
) {

    const std::uint32_t
        attn_norm_relative =
            layer
            * 2u
            * NORM_ROW_BYTES;


    const std::uint32_t
        ffn_norm_relative =
            attn_norm_relative
            + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        Serial.printf(
            "NORM_LOAD: FAIL|"
            "L%u|pos=%u\n",
            static_cast<unsigned>(
                layer
            ),
            static_cast<unsigned>(
                position
            )
        );

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::QProj,
            normed_rt,
            q_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::KProj,
            normed_rt,
            k_rt
        )
        ||
        !runMatrix(
            layer,
            xp::MatrixFamily::VProj,
            normed_rt,
            v_rt
        )
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


    helloAttentionFromCache(
        layer,
        position
    );


    if (
        !runMatrix(
            layer,
            xp::MatrixFamily::OProj,
            attention_rt,
            o_rt
        )
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    return finiteVector(hidden_rt,cfg::kDModel);
}

bool rf_attention_early(
    std::uint32_t layer,
    std::size_t position
) {

    if (
        layer < 1u
        ||
        layer > 4u
    ) {
        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer * 2u * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {
        return false;
    }


    ResolvedMatrix q{};
    ResolvedMatrix k{};
    ResolvedMatrix v{};
    ResolvedMatrix o{};
    ResolvedMatrix gate{};
    ResolvedMatrix up{};
    ResolvedMatrix down{};


#define EARLY_RESOLVE(FAMILY, DEST) \
    if (!resolveMatrix( \
            layer, \
            xp::MatrixFamily::FAMILY, \
            g_sd_scratch, \
            SD_SCRATCH_BYTES, \
            &DEST)) { \
        return false; \
    }


    EARLY_RESOLVE(QProj, q)
    EARLY_RESOLVE(KProj, k)
    EARLY_RESOLVE(VProj, v)
    EARLY_RESOLVE(OProj, o)
    EARLY_RESOLVE(GateProj, gate)
    EARLY_RESOLVE(UpProj, up)
    EARLY_RESOLVE(DownProj, down)


#undef EARLY_RESOLVE


    if (
        q.location != xp::MatrixLocation::Psram
        ||
        k.location != xp::MatrixLocation::Psram
        ||
        v.location != xp::MatrixLocation::Psram
        ||
        o.location != xp::MatrixLocation::Psram
        ||
        gate.location != xp::MatrixLocation::Psram
        ||
        up.location != xp::MatrixLocation::Psram
        ||
        down.location != xp::MatrixLocation::Psram
    ) {
        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
    const std::size_t q_split =
        q.spec->rows / 2;

    dualDrainDone();

    dual_job.kind = DualJobKind::Rows;
    dual_job.matrix = q;
    dual_job.input = normed_rt;
    dual_job.output = q_rt;
    dual_job.begin = q_split;
    dual_job.end = q.spec->rows;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool q0 =
        dualMatvecRows(
            q,
            normed_rt,
            q_rt,
            0,
            q_split
        );

    const bool q1 =
        dualWait();

    if (!q0 || !q1) {
        return false;
    }


     
    dualDrainDone();

    dual_job.kind = DualJobKind::Full;
    dual_job.matrix = v;
    dual_job.input = normed_rt;
    dual_job.output = v_rt;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool k_ok =
        pipeMatvec(
            k,
            normed_rt,
            k_rt
        );

    const bool v_ok =
        dualWait();

    if (!k_ok || !v_ok) {
        return false;
    }


    helloApplyRopeQk(position);
    helloStoreCurrentKv(layer, position);


     
    const std::size_t head_split =
        cfg::kNHeads / 2;

    dualDrainDone();

    dual_job.kind =
        DualJobKind::Attention;

    dual_job.layer =
        layer;

    dual_job.position =
        position;

    dual_job.begin =
        head_split;

    dual_job.end =
        cfg::kNHeads;

    dual_job.ok =
        false;

    xSemaphoreGive(dual_start);

    dualAttentionHeads(
        layer,
        position,
        0,
        head_split,
        hello_attn_scores
    );

    if (!dualWait()) {
        return false;
    }


     
    const std::size_t o_split =
        o.spec->rows / 2;

    dualDrainDone();

    dual_job.kind = DualJobKind::Rows;
    dual_job.matrix = o;
    dual_job.input = attention_rt;
    dual_job.output = o_rt;
    dual_job.begin = o_split;
    dual_job.end = o.spec->rows;
    dual_job.ok = false;

    xSemaphoreGive(dual_start);

    const bool o0 =
        dualMatvecRows(
            o,
            attention_rt,
            o_rt,
            0,
            o_split
        );

    const bool o1 =
        dualWait();

    if (!o0 || !o1) {
        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {
        hidden_rt[i] += o_rt[i];
    }


    return finiteVector(hidden_rt,cfg::kDModel);
}

bool rf_attention11(
    std::size_t position, const ResolvedMatrix& q
) {

    constexpr std::uint32_t layer =
        11u;


    if (
        !app_tail_ready
        ||
        !cached_o11_valid
        ||
        !dual_worker_task
        ||
        !dual_start
        ||
        !dual_done
    ) {

        return false;
    }


    const std::uint32_t attn_norm_relative =
        layer
        * 2u
        * NORM_ROW_BYTES;

    const std::uint32_t ffn_norm_relative =
        attn_norm_relative
        + NORM_ROW_BYTES;


    if (
        !loadNormRow(
            attn_norm_relative,
            attn_norm_weight_rt
        )
        ||
        !loadNormRow(
            ffn_norm_relative,
            ffn_norm_weight_rt
        )
    ) {

        return false;
    }


    runtimeRmsNorm(
        hidden_rt,
        attn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     

    if (
        !pipeMatvec(
            q,
            normed_rt,
            q_rt
        )
    ) {

        return false;
    }


     
     
     

    if (
        !pipeMatvec(
            app_tail_k11,
            normed_rt,
            k_rt
        )
    ) {

        return false;
    }


     
     
     
     

    const std::size_t v_split =
        app_tail_v11.spec->rows
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        app_tail_v11;

    dual_job.input =
        normed_rt;

    dual_job.output =
        v_rt;

    dual_job.begin =
        v_split;

    dual_job.end =
        app_tail_v11.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool v_main_ok =
        dualMatvecRows(
            app_tail_v11,
            normed_rt,
            v_rt,
            0u,
            v_split
        );


    const bool v_worker_ok =
        dualWait();


    if (
        !v_main_ok
        ||
        !v_worker_ok
    ) {

        return false;
    }


    helloApplyRopeQk(
        position
    );


    helloStoreCurrentKv(
        layer,
        position
    );


     
     
     
     

    const std::size_t head_split =
        cfg::kNHeads
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Attention;

    dual_job.layer =
        layer;

    dual_job.position =
        position;

    dual_job.begin =
        head_split;

    dual_job.end =
        cfg::kNHeads;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    dualAttentionHeads(
        layer,
        position,
        0u,
        head_split,
        hello_attn_scores
    );


    if (
        !dualWait()
    ) {

        return false;
    }


     
     
     
     

    const ResolvedMatrix o =
        cached_o11_matrix;

    const std::size_t o_split =
        o.spec->rows
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        o;

    dual_job.input =
        attention_rt;

    dual_job.output =
        o_rt;

    dual_job.begin =
        o_split;

    dual_job.end =
        o.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool o_main_ok =
        dualMatvecRows(
            o,
            attention_rt,
            o_rt,
            0u,
            o_split
        );


    const bool o_worker_ok =
        dualWait();


    if (
        !o_main_ok
        ||
        !o_worker_ok
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            o_rt[i];
    }


    return finiteVector(hidden_rt,cfg::kDModel);
}
