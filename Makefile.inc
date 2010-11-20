COMP    = stutter
FRACT   = diffuse lsys mrcm ifs mandel julia
CHAOS   = gen1d bifur1d phase1d henon henbif henwarp \
          lorenz rossler mg hencon gsw predprey lotka
COMPSYS = ca life hp boids termites vants sipd eipd \
          assoc hopfield
ADAPT   = gastring gabump gatask gasurf gaipd zcs zcscup mlp

DEMOS   = $(COMP) $(FRACT) $(CHAOS) $(COMPSYS) $(ADAPT)

