#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=
else
COMPARISON_BUILD=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=../lab5_uart.c ../ir/IRremote.c ../ir/irRecv.c ../ir/irSend.c ../ir/ir_Sony.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1472/lab5_uart.o ${OBJECTDIR}/_ext/43900888/IRremote.o ${OBJECTDIR}/_ext/43900888/irRecv.o ${OBJECTDIR}/_ext/43900888/irSend.o ${OBJECTDIR}/_ext/43900888/ir_Sony.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1472/lab5_uart.o.d ${OBJECTDIR}/_ext/43900888/IRremote.o.d ${OBJECTDIR}/_ext/43900888/irRecv.o.d ${OBJECTDIR}/_ext/43900888/irSend.o.d ${OBJECTDIR}/_ext/43900888/ir_Sony.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1472/lab5_uart.o ${OBJECTDIR}/_ext/43900888/IRremote.o ${OBJECTDIR}/_ext/43900888/irRecv.o ${OBJECTDIR}/_ext/43900888/irSend.o ${OBJECTDIR}/_ext/43900888/ir_Sony.o

# Source Files
SOURCEFILES=../lab5_uart.c ../ir/IRremote.c ../ir/irRecv.c ../ir/irSend.c ../ir/ir_Sony.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MX250F128B
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1472/lab5_uart.o: ../lab5_uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/lab5_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/lab5_uart.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/lab5_uart.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/lab5_uart.o.d" -o ${OBJECTDIR}/_ext/1472/lab5_uart.o ../lab5_uart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/IRremote.o: ../ir/IRremote.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/IRremote.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/IRremote.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/IRremote.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/IRremote.o.d" -o ${OBJECTDIR}/_ext/43900888/IRremote.o ../ir/IRremote.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/irRecv.o: ../ir/irRecv.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/irRecv.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/irRecv.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/irRecv.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/irRecv.o.d" -o ${OBJECTDIR}/_ext/43900888/irRecv.o ../ir/irRecv.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/irSend.o: ../ir/irSend.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/irSend.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/irSend.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/irSend.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/irSend.o.d" -o ${OBJECTDIR}/_ext/43900888/irSend.o ../ir/irSend.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/ir_Sony.o: ../ir/ir_Sony.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/ir_Sony.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/ir_Sony.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/ir_Sony.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/ir_Sony.o.d" -o ${OBJECTDIR}/_ext/43900888/ir_Sony.o ../ir/ir_Sony.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
else
${OBJECTDIR}/_ext/1472/lab5_uart.o: ../lab5_uart.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/1472" 
	@${RM} ${OBJECTDIR}/_ext/1472/lab5_uart.o.d 
	@${RM} ${OBJECTDIR}/_ext/1472/lab5_uart.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1472/lab5_uart.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/1472/lab5_uart.o.d" -o ${OBJECTDIR}/_ext/1472/lab5_uart.o ../lab5_uart.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/IRremote.o: ../ir/IRremote.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/IRremote.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/IRremote.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/IRremote.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/IRremote.o.d" -o ${OBJECTDIR}/_ext/43900888/IRremote.o ../ir/IRremote.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/irRecv.o: ../ir/irRecv.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/irRecv.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/irRecv.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/irRecv.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/irRecv.o.d" -o ${OBJECTDIR}/_ext/43900888/irRecv.o ../ir/irRecv.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/irSend.o: ../ir/irSend.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/irSend.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/irSend.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/irSend.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/irSend.o.d" -o ${OBJECTDIR}/_ext/43900888/irSend.o ../ir/irSend.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
${OBJECTDIR}/_ext/43900888/ir_Sony.o: ../ir/ir_Sony.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/_ext/43900888" 
	@${RM} ${OBJECTDIR}/_ext/43900888/ir_Sony.o.d 
	@${RM} ${OBJECTDIR}/_ext/43900888/ir_Sony.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/43900888/ir_Sony.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/_ext/43900888/ir_Sony.o.d" -o ${OBJECTDIR}/_ext/43900888/ir_Sony.o ../ir/ir_Sony.c    -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD) 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mdebugger -D__MPLAB_DEBUGGER_PK3=1 -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)    -mreserve=boot@0x1FC00490:0x1FC00BEF  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_PK3=1,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}          -DXPRJ_default=$(CND_CONF)    $(COMPARISON_BUILD)  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"
	${MP_CC_DIR}\\xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/protoThreads_1_1.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
