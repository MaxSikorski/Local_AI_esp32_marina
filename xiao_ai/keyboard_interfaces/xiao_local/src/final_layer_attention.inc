bool bp_l11(
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


    runtimeRmsNorm(
        hidden_rt,
        ffn_norm_weight_rt,
        normed_rt,
        cfg::kDModel
    );


     
     
     

    ResolvedMatrix gate{};
    ResolvedMatrix up{};
    ResolvedMatrix down{};


    if (
        !resolveMatrix(
            layer,
            xp::MatrixFamily::GateProj,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &gate
        )
        ||
        !resolveMatrix(
            layer,
            xp::MatrixFamily::UpProj,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &up
        )
        ||
        !resolveMatrix(
            layer,
            xp::MatrixFamily::DownProj,
            g_sd_scratch,
            SD_SCRATCH_BYTES,
            &down
        )
    ) {

        return false;
    }


    if (
        gate.location
            != xp::MatrixLocation::Flash
        ||
        up.location
            != xp::MatrixLocation::Flash
        ||
        down.location
            != xp::MatrixLocation::Flash
    ) {

        return false;
    }


     
     
     

    dualDrainDone();


    dual_job.kind =
        DualJobKind::Full;

    dual_job.matrix =
        up;

    dual_job.input =
        normed_rt;

    dual_job.output =
        up_rt;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool gate_ok =
        pipeMatvec(
            gate,
            normed_rt,
            gate_rt
        );


    const bool up_ok =
        dualWait();


    if (
        !gate_ok
        ||
        !up_ok
    ) {

        return false;
    }


     
     
     

    const std::size_t swiglu_split =
        cfg::kDff
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::SwiGlu;

    dual_job.begin =
        swiglu_split;

    dual_job.end =
        cfg::kDff;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    for (
        std::size_t i = 0;
        i < swiglu_split;
        ++i
    ) {

        const float g =
            gate_rt[i];


        gate_rt[i] =
            (
                g
                /
                (
                    1.0f
                    + expf(-g)
                )
            )
            * up_rt[i];
    }


    if (
        !dualWait()
    ) {

        return false;
    }


     
     
     

    const std::size_t down_split =
        down.spec->rows
        / 2u;


    dualDrainDone();


    dual_job.kind =
        DualJobKind::Rows;

    dual_job.matrix =
        down;

    dual_job.input =
        gate_rt;

    dual_job.output =
        down_rt;

    dual_job.begin =
        down_split;

    dual_job.end =
        down.spec->rows;

    dual_job.ok =
        false;


    xSemaphoreGive(
        dual_start
    );


    const bool down_main_ok =
        dualMatvecRows(
            down,
            gate_rt,
            down_rt,
            0u,
            down_split
        );


    const bool down_worker_ok =
        dualWait();


    if (
        !down_main_ok
        ||
        !down_worker_ok
    ) {

        return false;
    }


    for (
        std::size_t i = 0;
        i < cfg::kDModel;
        ++i
    ) {

        hidden_rt[i] +=
            down_rt[i];
    }


    pipe_next_q.valid =
        false;


    return finiteVector(
        hidden_rt,
        cfg::kDModel
    );
}
