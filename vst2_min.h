// Définitions minimales VST2 écrites from scratch (compatibles ABI), sans le SDK Steinberg.
#pragma once
#include <cstdint>
struct AEffect;
typedef intptr_t (*audioMasterCallback)(AEffect*, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef intptr_t (*AEffectDispatcherProc)(AEffect*, int32_t opcode, int32_t index, intptr_t value, void* ptr, float opt);
typedef void (*AEffectProcessProc)(AEffect*, float** inputs, float** outputs, int32_t sampleFrames);
typedef void (*AEffectProcessDoubleProc)(AEffect*, double** inputs, double** outputs, int32_t sampleFrames);
typedef void (*AEffectSetParameterProc)(AEffect*, int32_t index, float parameter);
typedef float (*AEffectGetParameterProc)(AEffect*, int32_t index);

struct AEffect {
    int32_t magic;
    AEffectDispatcherProc dispatcher;
    AEffectProcessProc process;
    AEffectSetParameterProc setParameter;
    AEffectGetParameterProc getParameter;
    int32_t numPrograms, numParams, numInputs, numOutputs, flags;
    intptr_t resvd1, resvd2;
    int32_t initialDelay, realQualities, offQualities;
    float ioRatio;
    void* object;
    void* user;
    int32_t uniqueID, version;
    AEffectProcessProc processReplacing;
    AEffectProcessDoubleProc processDoubleReplacing;
    char future[56];
};

enum {
    kEffectMagic = 0x56737450, // 'VstP'
    effOpen = 0, effClose, effSetProgram, effGetProgram, effSetProgramName, effGetProgramName,
    effGetParamLabel, effGetParamDisplay, effGetParamName,
    effSetSampleRate = 10, effSetBlockSize, effMainsChanged,
    effEditGetRect = 13, effEditOpen, effEditClose, effEditIdle = 19,
    effIdentify = 22, kIdentifyNvEf = 0x4e764566, // 'NvEf' : réponse obligatoire pour les hôtes VST 2.0 anciens
    effGetPlugCategory = 35, effGetEffectName = 45, effGetVendorString = 47,
    effGetProductString, effGetVendorVersion, effCanDo = 51, effGetVstVersion = 58,
    effFlagsHasEditor = 1, effFlagsCanReplacing = 1 << 4,
    kPlugCategEffect = 1
};

struct ERect { short top, left, bottom, right; };
