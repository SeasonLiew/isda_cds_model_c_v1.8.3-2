/*
 * ISDA CDS Standard Model
 *
 * Copyright (C) 2009 International Swaps and Derivatives Association, Inc.
 * Developed and supported in collaboration with Markit
 * 
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the ISDA CDS Standard Model Public License.
 */

#include "version.h"
#include "macros.h"
#include "cerror.h"
#include "tcurve.h"
#include "cdsone.h"
#include "convert.h"
#include "zerocurve.h"
#include "cds.h"
#include "cxzerocurve.h"
#include "dateconv.h"
#include "date_sup.h"
#include "busday.h"
#include "ldate.h"

#define SET_DEFAULT(variable, value) \
    if (variable == NULL) variable = strdup(value) \


/*
***************************************************************************
** Build IR zero curve.
***************************************************************************
*/



TCurve* BuildExampleZeroCurve()
{
    static char  *routine = "BuildExampleZeroCurve";
    TCurve       *zc = NULL;
    char         *types = "MMMMMMSSSSSSSSSSSSSSS";
    char         *expiries[21] = {"1M", "2M", "3M", "6M", "9M", "1Y", "2Y", "3Y", "4Y", "5Y", "6Y", "7Y", "8Y", "9Y","10Y", "11Y", "12Y", "15Y", "20Y", "25Y","30Y"};
    TDate        *dates = NULL;
    double        rates[21] = {0.00445,0.009488,0.012337,0.017762,0.01935,0.020838,0.01652,0.02018,0.023033,0.02525,0.02696,0.02825,0.02931,0.03017,0.03092,0.0316,0.03231,0.03367,0.03419,0.03411,0.03412};
    TDate         baseDate;
    long          mmDCC;
    TDateInterval ivl;
    long          dcc;
    double        freq;
    char          badDayConv = 'M';
    char         *holidays = "None";
    int           i, n;

    baseDate = JpmcdsDate(2024, 11, 21);

    if (JpmcdsStringToDayCountConv("Act/360", &mmDCC) != SUCCESS)
        goto done;
    
    if (JpmcdsStringToDayCountConv("30/360", &dcc) != SUCCESS)
        goto done;

    if (JpmcdsStringToDateInterval("6M", routine, &ivl) != SUCCESS)
        goto done;

    if (JpmcdsDateIntervalToFreq(&ivl, &freq) != SUCCESS)
        goto done;

    n = strlen(types);

    dates = NEW_ARRAY(TDate, n);
    for (i = 0; i < n; i++)
    {
        TDateInterval tmp;

        if (JpmcdsStringToDateInterval(expiries[i], routine, &tmp) != SUCCESS)
        {
            JpmcdsErrMsg ("%s: invalid interval for element[%d].\n", routine, i);
            goto done;
        }
        
        if (JpmcdsDateFwdThenAdjust(baseDate, &tmp, JPMCDS_BAD_DAY_NONE, "None", dates+i) != SUCCESS)
        {
            JpmcdsErrMsg ("%s: invalid interval for element[%d].\n", routine, i);
            goto done;
        }
    }
 
    printf("calling JpmcdsBuildIRZeroCurve...\n");
    zc = JpmcdsBuildIRZeroCurve(
            baseDate,
            types,
            dates,
            rates,
            n,
            mmDCC,
            (long) freq,
            (long) freq,
            dcc,
            dcc,
            badDayConv,
            holidays);
done:
    FREE(dates);
    return zc;
}

/*
***************************************************************************
** Spread curve.
***************************************************************************
*/
TCurve* BuildSpreadZeroCurve(TCurve  *discCurve){

    static char   *routine = "CDS_CleanSpreadCurveBuild";
    int            status = FAILURE;
    
    TDate          today;               /* 1 */
    TDate          startDate;
    TDate          stepinDate;
    TDate          cashSettleDate;
    //TDate         *dates = NULL;        /* 5 */
    //double        *rates = NULL;
    
    long           payAccOnDefault;
    char          *couponInterval = NULL;
    char          *stubType = NULL;    /* 10 */
    char          *paymentDcc = NULL;
    char          *badDayConv = NULL;
    char          *holidays = NULL;
    double         recoveryRate;
    char          *name = NULL;
    
    TCurve        *spreadCurve = NULL;
    char          *handle1 = NULL;
    char          *handle2 = NULL;

    int            i;
    int            n;
    TStubMethod    stub;
    long           dcc;
    TDateInterval  ivl;

    
    today=JpmcdsDate(2024, 11, 21); //a1
    startDate=JpmcdsDate(2024, 11, 21); //a2
    stepinDate=JpmcdsDate(2024, 11, 22); //a3
    cashSettleDate=JpmcdsDate(2024, 11, 26); //a4

    TDate dates[] = { //a5
        JpmcdsDate(2025, 5, 22),//0.5
        JpmcdsDate(2025, 11, 21),//1
        JpmcdsDate(2026, 11, 21),//2
        JpmcdsDate(2027, 11, 21),//3
        JpmcdsDate(2028, 11, 20),//4
        JpmcdsDate(2029, 11, 20),//5
        JpmcdsDate(2031, 11, 20),//7
        JpmcdsDate(2034, 11, 19),//10
        JpmcdsDate(2039, 11, 18),//15
        JpmcdsDate(2044, 11, 16),//20
        JpmcdsDate(2054, 11, 14),//30
    };

    

    double rates[] =    {0.003703519
                        ,0.004480285
                        ,0.006753535
                        ,0.009355809
                        ,0.011996311
                        ,0.014686373
                        ,0.018078505
                        ,0.020623662
                        ,0.022180302
                        ,0.022723001
                        ,0.02336132};

    n=sizeof(rates)/8;
    

    TBoolean includes[] = {1, 1, 1, 1, 1, 1, 1, 1,1,1,1};//a7
    payAccOnDefault=1; //a8
    couponInterval="Q"; //a9
    stubType="F/S"; //a10
    paymentDcc="ACT/360"; //a11
    badDayConv="F"; //a12
    holidays="None"; //a13
    recoveryRate=0.4; //a15
    name="Clean"; //a16

    
    SET_DEFAULT(stubType, "f/s");
    if (JpmcdsStringToStubMethod(stubType, &stub) != SUCCESS)
        goto done;

    SET_DEFAULT(paymentDcc, "ACT/360");
    if (JpmcdsStringToDayCountConv(paymentDcc, &dcc) != SUCCESS)
        goto done;

    SET_DEFAULT(badDayConv, "N");
    if (JpmcdsBadDayConvValid(routine, badDayConv[0]) != SUCCESS)
        goto done;

    if (JpmcdsStringToDateInterval(couponInterval, routine, &ivl) != SUCCESS)
        goto done;

   
    
     spreadCurve = JpmcdsCleanSpreadCurve(today, //20241121 
                                          discCurve,//
                                          startDate,//
                                          stepinDate,
                                          cashSettleDate,
                                          n,
                                          dates,
                                          rates,
                                          includes,
                                          recoveryRate,
                                          (TBoolean)(payAccOnDefault),
                                          &ivl,
                                          dcc,
                                          &stub,
                                          badDayConv[0],
                                          holidays);

        


    return spreadCurve;
done:
printf(1);

}

/*



/*
***************************************************************************
** Main function.
***************************************************************************
*/
int main(int argc, char** argv)
{
    int     status = 1;
    char    version[256];
    char  **lines = NULL;
    int     i;
    TCurve *zerocurve = NULL;

    if (JpmcdsVersionString(version) != SUCCESS)
        goto done;

    /* print library version */
    printf("starting78979...\n");
    printf("%s\n", version);
    
    /* enable logging */
    printf("enabling logging...\n");
    if (JpmcdsErrMsgEnableRecord(20, 128) != SUCCESS) /* ie. 20 lines, each of max length 128 */
        goto done;

    /* construct IR zero curve */
    printf("building zero curve...\n");
    zerocurve = BuildExampleZeroCurve();// the discount curve
    for(i=0;i<64;i++){
        printf("zero->%f,",zerocurve->fArray[i].fRate);
        printf("zero->%ld",zerocurve->fArray[i].fDate);
        printf("\n");
    }


    if (zerocurve == NULL)
        goto done;

    TCurve* exampl= BuildSpreadZeroCurve(zerocurve); // calls JpmcdsCleanSpreadCurve to bootstrap the cds spread in the sector xlsx file, out put bootstrapped file
    for(i=0;i<11;i++){
        printf("%ld->%f\n",exampl->fArray[i].fDate,exampl->fArray[i].fRate);
        
    }

    double result = JpmcdsZeroPrice(exampl, JpmcdsDate(2054, 11, 14)); // from bootstrapped cds curve PD.
    printf("pdis->%lf\n",result);









done:
    if (status != 0)
        printf("\n*** ERROR ***\n");

    /* print error log contents */
    printf("\n");
    printf("Error log contains:\n");
    printf("------------------:\n");

    lines = JpmcdsErrGetMsgRecord();
    if (lines == NULL)
        printf("(no log contents)\n");
    else
    {
        for(i = 0; lines[i] != NULL; i++)
        {
            if (strcmp(lines[i],"") != 0)
                printf("%s\n", lines[i]);
        }
    }

    FREE(zerocurve);
    return status;
}
