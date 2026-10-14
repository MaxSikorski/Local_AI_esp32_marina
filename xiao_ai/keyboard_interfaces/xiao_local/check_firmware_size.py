from pathlib import Path
Import('env')


def check_layout(source, target, env):
    binary = Path(env.subst('$BUILD_DIR')) / (env.subst('$PROGNAME') + '.bin')
    limit = 0x6F000
    if binary.stat().st_size > limit:
        raise RuntimeError('Unsafe local UI image: app overlaps fixed model cache at flash 0x7F000. Do not upload. Reduce app below 454656 bytes or redesign/rebuild cache placement.')


env.AddPostAction('$BUILD_DIR/${PROGNAME}.bin', check_layout)
env.AddPreAction('upload', check_layout)
