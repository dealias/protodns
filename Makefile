TRI = $(HOME)/tri
ARCH = unix
FFT = fft
POLL = poll

include $(TRI)/config/Common

vpath %.cc $(TRI)/nw
INCL = -I. -I$(TRI) -I$(TRI)/nw -I-

TRIAD = dns NWave Cartesian rfft $(FFT) $(CORE) $(UTILS)

DEPEND = $(TRIAD)

include $(TRI)/config/Rules

