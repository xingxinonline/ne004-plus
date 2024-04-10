#ifndef __MFCC_CORE_H__
#define __MFCC_CORE_H__
#include "mfcc_type.h"
#include "mfcc_mel.h"

/*MFCC核心计算模块*/
void ComputeMfccCore(TMFCC* mfcc, BaseFloat* feature);
#endif