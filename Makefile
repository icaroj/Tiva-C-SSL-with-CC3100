DEBUG=1
PART=TM4C1294NCPDT

# root dirs
ROOT=../tivaware
CC3100_SDK_ROOT=../cc3100-sdk

include ${ROOT}/makedefs

# source files macro
VPATH=.
VPATH+=./platform
VPATH+=./common
VPATH+=./simplelink/include
VPATH+=./simplelink
VPATH+=./simplelink/source
VPATH+=${ROOT}/utils
VPATH+=${CC3100_SDK_ROOT}

# header files macro
IPATH=.
IPATH+=./platform
IPATH+=./common
IPATH+=./simplelink/include
IPATH+=./simplelink
IPATH+=./simplelink/source
IPATH+=${ROOT}

# make all
all: ${COMPILER}
all: ${COMPILER}/ssl.axf

# make clean
clean:
	@rm -rf ${COMPILER} ${wildcard *~}

# target dir
${COMPILER}:
	@mkdir -p ${COMPILER}

# build rules
# project
${COMPILER}/ssl.axf: ${COMPILER}/device.o
${COMPILER}/ssl.axf: ${COMPILER}/driver.o
${COMPILER}/ssl.axf: ${COMPILER}/flowcont.o
${COMPILER}/ssl.axf: ${COMPILER}/fs.o
${COMPILER}/ssl.axf: ${COMPILER}/netapp.o
${COMPILER}/ssl.axf: ${COMPILER}/netcfg.o
${COMPILER}/ssl.axf: ${COMPILER}/nonos.o
${COMPILER}/ssl.axf: ${COMPILER}/socket.o
${COMPILER}/ssl.axf: ${COMPILER}/spawn.o
${COMPILER}/ssl.axf: ${COMPILER}/wlan.o

${COMPILER}/ssl.axf: ${COMPILER}/board.o
${COMPILER}/ssl.axf: ${COMPILER}/cli_uart.o
${COMPILER}/ssl.axf: ${COMPILER}/spi.o

${COMPILER}/ssl.axf: ${COMPILER}/ustdlib.o
${COMPILER}/ssl.axf: ${COMPILER}/uartstdio.o

${COMPILER}/ssl.axf: ${COMPILER}/vectors.o
${COMPILER}/ssl.axf: ${COMPILER}/main.o
${COMPILER}/ssl.axf: ${COMPILER}/sl_config.o

${COMPILER}/ssl.axf: ${ROOT}/driverlib/${COMPILER}/libdriver.a
${COMPILER}/ssl.axf: TM4C1294NCPDT.ld

SCATTERgcc_ssl=TM4C1294NCPDT.ld
ENTRY_ssl=startup
# Tells the preprocessor what device is being compiled
CFLAGSgcc=-DTARGET_IS_TM4C129_RA1
# Console
CFLAGSgcc+=-D_USE_CLI_
# disabling some warnings...
CFLAGSgcc+=-Wno-missing-braces

ifneq (${MAKECMDGOALS},clean)
-include ${wildcard ${COMPILER}/*.d} __dummy__
endif
