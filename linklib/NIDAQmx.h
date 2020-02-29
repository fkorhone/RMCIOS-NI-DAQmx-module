// Nidaqmx C reference help
// https://zone.ni.com/reference/en-XX/help/370471AM-01/
#ifndef ___nidaqmx_h___
#define ___nidaqmx_h___

#include <inttypes.h>

#define DAQmx_Val_Auto               -1
#define DAQmx_Val_GroupByChannel      0 
#define DAQmx_Val_GroupByScanNumber   1  
#define DAQmx_Val_Cfg_Default        -1
#define DAQmx_Val_RSE                10083 
#define DAQmx_Val_NRSE               10078
#define DAQmx_Val_Diff               10106
#define DAQmx_Val_PseudoDiff         12529 
#define DAQmx_Val_Volts              10348
#define DAQmx_Val_Rising             10280 
#define DAQmx_Val_Falling            10171 
#define DAQmx_Val_FiniteSamps        10178 
#define DAQmx_Val_High               10192
#define DAQmx_Val_Low                10214 
#define DAQmx_Val_Tristate           10310
#define DAQmx_Val_NoChange           10160 
#define DAQmx_Val_GroupByChannel     0
#define DAQmx_Val_Hz                 10373
#define DAQmx_Val_ContSamps          10123
#define DAQmx_Val_CountUp            10128 
#define DAQmx_Val_CountDown          10124 
#define DAQmx_Val_ExtControlled      10326 
#define DAQmx_Val_ChanPerLine        0

#define DAQmxFailed(error)           ((error)<0)

typedef void *TaskHandle;
typedef uint8_t uInt8;
typedef int16_t int16;
typedef uint16_t uInt16;
typedef int32_t int32;
typedef uint32_t uInt32;
typedef float float32;
typedef double float64;
typedef int64_t int64;
typedef uint64_t uInt64;
typedef uint32_t bool32;

int32_t __stdcall DAQmxCreateTask(const char *, void *);
int32_t __stdcall DAQmxStartTask(void *);
int32_t __stdcall DAQmxStopTask(void *);
int32_t __stdcall DAQmxReadAnalogF64(void *, int32_t, double, uint32_t, double *, uint32_t, int32_t *, uint32_t *);
int32_t __stdcall DAQmxCreateAIVoltageChan(void *, const char *, const char *, int32_t, double, double, int32_t, const char *);
int32_t __stdcall DAQmxCfgSampClkTiming(void *, const char *, double, int32_t, int32_t, uint64_t);
int32_t __stdcall DAQmxGetExtendedErrorInfo(char *, uint32_t);
int32_t __stdcall DAQmxClearTask(void *);
int32_t __stdcall DAQmxCreateAOVoltageChan(void *, const char *, const char *, double, double, int32_t, const char *);
int32_t __stdcall DAQmxWriteAnalogScalarF64(void * , uint32_t, double, double, uint32_t *);
int32_t __stdcall DAQmxWriteCtrFreq(void * , int32_t, uint32_t, double, uint32_t, const double *, const double *, int32_t *, uint32_t *);
int32_t __stdcall DAQmxCreateCOPulseChanFreq(void * , const char *, const char *, int32_t, int32_t, double, double, double);
int32_t __stdcall DAQmxCfgImplicitTiming(void * , int32_t, uint64_t);
int32_t __stdcall DAQmxCreateCICountEdgesChan(void * , const char *, const char *, int32_t, uint32_t, int32_t);
int32_t __stdcall DAQmxSetCICountEdgesTerm(void * , const char *, const char *);
int32_t __stdcall DAQmxReadCounterScalarU32(void * , double, uint32_t *, uint32_t *);
int32_t __stdcall DAQmxCreateDOChan(void * , const char *, const char *, int32_t);
int32_t __stdcall DAQmxWriteDigitalLines(void * , int32_t, uint32_t, double, uint32_t, const uint8_t *, int32_t *, uint32_t *);

#endif
