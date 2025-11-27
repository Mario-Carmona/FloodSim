#ifdef __unix__
#include <stdio.h>
#endif

#include "PrintTypesAsASN1.h"

#ifdef __linux__
#include <pthread.h>

static pthread_mutex_t g_printing_mutex = PTHREAD_MUTEX_INITIALIZER;

#endif

void PrintASN1T_Real32(const char *paramName, const asn1SccT_Real32 *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-Real32 ::= ", paramName);
    printf("%s ", paramName);
    printf("%f", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_UReal32(const char *paramName, const asn1SccT_UReal32 *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-UReal32 ::= ", paramName);
    printf("%s ", paramName);
    printf("%f", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1SurfaceType(const char *paramName, const asn1SccSurfaceType *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s SurfaceType ::= ", paramName);
    printf("%s ", paramName);
    switch((*pData)) {
    case 0:
        printf("soil");
        break;
    case 1:
        printf("rock");
        break;
    case 2:
        printf("asphalt");
        break;
    case 3:
        printf("forest");
        break;
    case 4:
        printf("waterSurface");
        break;
    default:
        printf("Invalid value in ENUMERATED ((*pData))");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1CellState(const char *paramName, const asn1SccCellState *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s CellState ::= ", paramName);
    printf("%s ", paramName);
    printf("{");
    printf("elevation ");
    printf("%f", (*pData).elevation);
    printf(", ");
    printf("waterVolume ");
    printf("%f", (*pData).waterVolume);
    printf(", ");
    printf("surfaceType ");
    switch((*pData).surfaceType) {
    case 0:
        printf("soil");
        break;
    case 1:
        printf("rock");
        break;
    case 2:
        printf("asphalt");
        break;
    case 3:
        printf("forest");
        break;
    case 4:
        printf("waterSurface");
        break;
    default:
        printf("Invalid value in ENUMERATED ((*pData).surfaceType)");
    }
    printf("}");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1GridVector(const char *paramName, const asn1SccGridVector *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s GridVector ::= ", paramName);
    printf("%s ", paramName);
    {
        int i1;
        printf("{");
        for(i1=0; i1<(*pData).nCount; i1++) {
            if (i1) 
                printf(",");
            printf("{");
            printf("elevation ");
            printf("%f", (*pData).arr[i1].elevation);
            printf(", ");
            printf("waterVolume ");
            printf("%f", (*pData).arr[i1].waterVolume);
            printf(", ");
            printf("surfaceType ");
            switch((*pData).arr[i1].surfaceType) {
            case 0:
                printf("soil");
                break;
            case 1:
                printf("rock");
                break;
            case 2:
                printf("asphalt");
                break;
            case 3:
                printf("forest");
                break;
            case 4:
                printf("waterSurface");
                break;
            default:
                printf("Invalid value in ENUMERATED ((*pData).arr[i1].surfaceType)");
            }
            printf("}");
        }
        printf("}");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1Grid(const char *paramName, const asn1SccGrid *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s Grid ::= ", paramName);
    printf("%s ", paramName);
    printf("{");
    printf("width ");
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData).width);
    #else
    printf("%d", (*pData).width);
    #endif
    printf(", ");
    printf("height ");
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData).height);
    #else
    printf("%d", (*pData).height);
    #endif
    printf(", ");
    printf("cells ");
    {
        int i2;
        printf("{");
        for(i2=0; i2<(*pData).cells.nCount; i2++) {
            if (i2) 
                printf(",");
            printf("{");
            printf("elevation ");
            printf("%f", (*pData).cells.arr[i2].elevation);
            printf(", ");
            printf("waterVolume ");
            printf("%f", (*pData).cells.arr[i2].waterVolume);
            printf(", ");
            printf("surfaceType ");
            switch((*pData).cells.arr[i2].surfaceType) {
            case 0:
                printf("soil");
                break;
            case 1:
                printf("rock");
                break;
            case 2:
                printf("asphalt");
                break;
            case 3:
                printf("forest");
                break;
            case 4:
                printf("waterSurface");
                break;
            default:
                printf("Invalid value in ENUMERATED ((*pData).cells.arr[i2].surfaceType)");
            }
            printf("}");
        }
        printf("}");
    }
    printf("}");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1Coefficient(const char *paramName, const asn1SccCoefficient *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s Coefficient ::= ", paramName);
    printf("%s ", paramName);
    printf("%f", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1Duration(const char *paramName, const asn1SccDuration *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s Duration ::= ", paramName);
    printf("%s ", paramName);
    printf("%f", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1TimeStep(const char *paramName, const asn1SccTimeStep *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s TimeStep ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1FilePath(const char *paramName, const asn1SccFilePath *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s FilePath ::= ", paramName);
    printf("%s ", paramName);
    printf("\"%s\"", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1CellIndex(const char *paramName, const asn1SccCellIndex *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s CellIndex ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1Outflow(const char *paramName, const asn1SccOutflow *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s Outflow ::= ", paramName);
    printf("%s ", paramName);
    printf("{");
    printf("volume ");
    printf("%f", (*pData).volume);
    printf(", ");
    printf("neighbor ");
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData).neighbor);
    #else
    printf("%d", (*pData).neighbor);
    #endif
    printf("}");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1OutflowList(const char *paramName, const asn1SccOutflowList *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s OutflowList ::= ", paramName);
    printf("%s ", paramName);
    {
        int i3;
        printf("{");
        for(i3=0; i3<(*pData).nCount; i3++) {
            if (i3) 
                printf(",");
            printf("{");
            printf("volume ");
            printf("%f", (*pData).arr[i3].volume);
            printf(", ");
            printf("neighbor ");
            #if WORD_SIZE==8
            printf("%"PRId64, (*pData).arr[i3].neighbor);
            #else
            printf("%d", (*pData).arr[i3].neighbor);
            #endif
            printf("}");
        }
        printf("}");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1MooreNeighborList(const char *paramName, const asn1SccMooreNeighborList *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s MooreNeighborList ::= ", paramName);
    printf("%s ", paramName);
    {
        int i4;
        printf("{");
        for(i4=0; i4<(*pData).nCount; i4++) {
            if (i4) 
                printf(",");
            #if WORD_SIZE==8
            printf("%"PRId64, (*pData).arr[i4]);
            #else
            printf("%d", (*pData).arr[i4]);
            #endif
        }
        printf("}");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1GridCellNeighborList(const char *paramName, const asn1SccGridCellNeighborList *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s GridCellNeighborList ::= ", paramName);
    printf("%s ", paramName);
    {
        int i5;
        printf("{");
        for(i5=0; i5<(*pData).nCount; i5++) {
            if (i5) 
                printf(",");
            {
                int i6;
                printf("{");
                for(i6=0; i6<(*pData).arr[i5].nCount; i6++) {
                    if (i6) 
                        printf(",");
                    #if WORD_SIZE==8
                    printf("%"PRId64, (*pData).arr[i5].arr[i6]);
                    #else
                    printf("%d", (*pData).arr[i5].arr[i6]);
                    #endif
                }
                printf("}");
            }
        }
        printf("}");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1WaterIncrementEvent(const char *paramName, const asn1SccWaterIncrementEvent *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s WaterIncrementEvent ::= ", paramName);
    printf("%s ", paramName);
    printf("{");
    printf("targetCell ");
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData).targetCell);
    #else
    printf("%d", (*pData).targetCell);
    #endif
    printf("}");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1WaterIncrementEventList(const char *paramName, const asn1SccWaterIncrementEventList *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s WaterIncrementEventList ::= ", paramName);
    printf("%s ", paramName);
    {
        int i7;
        printf("{");
        for(i7=0; i7<(*pData).nCount; i7++) {
            if (i7) 
                printf(",");
            printf("{");
            printf("targetCell ");
            #if WORD_SIZE==8
            printf("%"PRId64, (*pData).arr[i7].targetCell);
            #else
            printf("%d", (*pData).arr[i7].targetCell);
            #endif
            printf("}");
        }
        printf("}");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1GridModification(const char *paramName, const asn1SccGridModification *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s GridModification ::= ", paramName);
    printf("%s ", paramName);
    printf("{");
    printf("targetCell ");
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData).targetCell);
    #else
    printf("%d", (*pData).targetCell);
    #endif
    printf(", ");
    printf("delta ");
    printf("%f", (*pData).delta);
    printf("}");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1GridModificationList(const char *paramName, const asn1SccGridModificationList *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s GridModificationList ::= ", paramName);
    printf("%s ", paramName);
    {
        int i8;
        printf("{");
        for(i8=0; i8<(*pData).nCount; i8++) {
            if (i8) 
                printf(",");
            printf("{");
            printf("targetCell ");
            #if WORD_SIZE==8
            printf("%"PRId64, (*pData).arr[i8].targetCell);
            #else
            printf("%d", (*pData).arr[i8].targetCell);
            #endif
            printf(", ");
            printf("delta ");
            printf("%f", (*pData).arr[i8].delta);
            printf("}");
        }
        printf("}");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1WaterVolume(const char *paramName, const asn1SccWaterVolume *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s WaterVolume ::= ", paramName);
    printf("%s ", paramName);
    printf("%f", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1TotalOutflow(const char *paramName, const asn1SccTotalOutflow *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s TotalOutflow ::= ", paramName);
    printf("%s ", paramName);
    printf("%f", (*pData));
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1MooreNeighborIndex(const char *paramName, const asn1SccMooreNeighborIndex *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s MooreNeighborIndex ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_Int32(const char *paramName, const asn1SccT_Int32 *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-Int32 ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_UInt32(const char *paramName, const asn1SccT_UInt32 *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-UInt32 ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_Runtime_Error(const char *paramName, const asn1SccT_Runtime_Error *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-Runtime-Error ::= ", paramName);
    printf("%s ", paramName);
    if ((*pData).kind == T_Runtime_Error_noerror_PRESENT) {
        printf("noerror:");
        #if WORD_SIZE==8
        printf("%"PRId64, (*pData).u.noerror);
        #else
        printf("%d", (*pData).u.noerror);
        #endif
    }
    else if ((*pData).kind == T_Runtime_Error_encodeerror_PRESENT) {
        printf("encodeerror:");
        #if WORD_SIZE==8
        printf("%"PRId64, (*pData).u.encodeerror);
        #else
        printf("%d", (*pData).u.encodeerror);
        #endif
    }
    else if ((*pData).kind == T_Runtime_Error_decodeerror_PRESENT) {
        printf("decodeerror:");
        #if WORD_SIZE==8
        printf("%"PRId64, (*pData).u.decodeerror);
        #else
        printf("%d", (*pData).u.decodeerror);
        #endif
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_Int8(const char *paramName, const asn1SccT_Int8 *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-Int8 ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_UInt8(const char *paramName, const asn1SccT_UInt8 *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-UInt8 ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_Boolean(const char *paramName, const asn1SccT_Boolean *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-Boolean ::= ", paramName);
    printf("%s ", paramName);
    printf("%s", (int)(*pData)?"TRUE":"FALSE");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1T_Null_Record(const char *paramName, const asn1SccT_Null_Record *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s T-Null-Record ::= ", paramName);
    printf("%s ", paramName);
    printf("{");
    printf("}");
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1PID_Range(const char *paramName, const asn1SccPID_Range *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s PID-Range ::= ", paramName);
    printf("%s ", paramName);
    #if WORD_SIZE==8
    printf("%"PRId64, (*pData));
    #else
    printf("%d", (*pData));
    #endif
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

void PrintASN1PID(const char *paramName, const asn1SccPID *pData)
{
    (void)paramName;
    (void)pData;
#ifdef __linux__
    pthread_mutex_lock(&g_printing_mutex);
#endif
#ifdef __unix__
    //printf("%s PID ::= ", paramName);
    printf("%s ", paramName);
    switch((*pData)) {
    case 0:
        printf("core");
        break;
    case 1:
        printf("io-gis");
        break;
    case 2:
        printf("ui");
        break;
    case 3:
        printf("env");
        break;
    default:
        printf("Invalid value in ENUMERATED ((*pData))");
    }
#endif
#ifdef __linux__
    pthread_mutex_unlock(&g_printing_mutex);
#endif
}

