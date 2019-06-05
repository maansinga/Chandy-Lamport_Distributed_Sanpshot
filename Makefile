# Root directory of your project
PROJDIR=${HOME}/DistributedSystems
SRC=${PROJDIR}/src
BINDIR=${PROJDIR}/bin
DUMPDIR=${PROJDIR}/dumps/
PROG=${BINDIR}/ChandyLamport

CONFIG=${PROJDIR}/config.txt

all:
	gcc \
		-DHOME="${HOME}"\
		-DPROJDIR="${PROJDIR}"\
		-DSRC="${SRC}"\
		-DBINDIR="${BINDIR}"\
		-DDUMPDIR="${DUMPDIR}"\
		-DPROG="${PROG}"\
		-DCONFIG="\"${CONFIG}\""\
		-std=c99\
		-pthread\
		-W\
		-g\
		${SRC}/*.c\
		-o ${PROG}

clean:
	rm ${PROG}