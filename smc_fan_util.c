#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <IOKit/IOKitLib.h>
#include "smc.h"

#ifndef DEBUG
#define DAEMON
#endif

// TODO:
// apply to macs with different numbers of fans
// improve augument parser
// improve error handling
// use json configuration file

void signal_handler(int signal)
{
    #ifdef DEBUG
    printf("daemon ended.\n");
    #endif
    smc_close();
    exit(EXIT_SUCCESS);
}

static void daemonize()
{
    pid_t pid;

    pid = fork();

    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    if (setsid() < 0)
    {
        exit(EXIT_FAILURE);
    }

    signal(SIGTERM, signal_handler);
    signal(SIGHUP, signal_handler);

    pid = fork();

    if (pid < 0)
    {
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        exit(EXIT_SUCCESS);
    }

    umask(0);

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

float getFloatFromVal(SMCVal_t val)
{
    float fval = -1.0f;

    if (val.dataSize > 0)
    {
        if (strcmp(val.dataType, DATATYPE_FLT) == 0 && val.dataSize == 4)
        {
            memcpy(&fval, val.bytes, sizeof(float));
        }
        else if (strcmp(val.dataType, DATATYPE_FPE2) == 0 && val.dataSize == 2)
        {
            fval = _strtof(val.bytes, val.dataSize, 2);
        }
        else if (strcmp(val.dataType, DATATYPE_UINT16) == 0 && val.dataSize == 2)
        {
            fval = (float) _strtoul((char *) val.bytes, val.dataSize, 10);
        }
        else if (strcmp(val.dataType, DATATYPE_UINT8) == 0 && val.dataSize == 1)
        {
            fval = (float) _strtoul((char *) val.bytes, val.dataSize, 10);
        }
    }

    return fval;
}

float getFloatFromKey(const char *key)
{

    kern_return_t result;
    SMCVal_t val;

    char key_[5] = {'\0'};
    snprintf(key_, 5, "%s", key);

    result = SMCReadKey(key_, &val);

    if (result != kIOReturnSuccess)
    {
        printf("Error: SMCReadKey() = %08x\n", result);
    }

    float fval = -1.0f;

    if (val.dataSize > 0)
    {
        if (strcmp(val.dataType, DATATYPE_FLT) == 0 && val.dataSize == 4)
        {
            memcpy(&fval, val.bytes, sizeof(float));
        }
        else if (strcmp(val.dataType, DATATYPE_FPE2) == 0 && val.dataSize == 2)
        {
            fval = _strtof(val.bytes, val.dataSize, 2);
        }
        else if (strcmp(val.dataType, DATATYPE_UINT16) == 0 && val.dataSize == 2)
        {
            fval = (float) _strtoul((char *) val.bytes, val.dataSize, 10);
        }
        else if (strcmp(val.dataType, DATATYPE_UINT8) == 0 && val.dataSize == 1)
        {
            fval = (float) _strtoul((char *) val.bytes, val.dataSize, 10);
        }
    }

    return fval;
}

kern_return_t SMCPrintFans(void)
{
    kern_return_t result;
    SMCVal_t val;
    UInt32Char_t key;
    int totalFans, i;

    result = SMCReadKey("FNum", &val);

    if (result != kIOReturnSuccess)
    {
        return kIOReturnError;
    }

    totalFans = _strtoul((char *) val.bytes, val.dataSize, 10);
    printf("Total fans in system: %d\n", totalFans);

    for (i = 0; i < totalFans; i++)
    {
        printf("\nFan #%d:\n", i);
        sprintf(key, "F%dID", i);
        SMCReadKey(key, &val);

        if (val.dataSize > 0)
        {
            printf("    Fan ID       : %s\n", val.bytes + 4);
        }

        sprintf(key, "F%dAc", i);
        SMCReadKey(key, &val);
        printf("    Actual speed : %.0f\n", getFloatFromVal(val));
        sprintf(key, "F%dMn", i);
        SMCReadKey(key, &val);
        printf("    Minimum speed: %.0f\n", getFloatFromVal(val));
        sprintf(key, "F%dMx", i);
        SMCReadKey(key, &val);
        printf("    Maximum speed: %.0f\n", getFloatFromVal(val));
        sprintf(key, "F%dSf", i);
        SMCReadKey(key, &val);
        printf("    Safe speed   : %.0f\n", getFloatFromVal(val));
        sprintf(key, "F%dTg", i);
        SMCReadKey(key, &val);
        printf("    Target speed : %.0f\n", getFloatFromVal(val));
        SMCReadKey("FS! ", &val);

        if (val.dataSize > 0)
        {
            if ((_strtoul((char *) val.bytes, 2, 16) & (1 << i)) == 0)
            {
                printf("    Mode         : auto\n");
            }
            else
            {
                printf("    Mode         : forced\n");
            }
        }
        else
        {
            sprintf(key, "F%dMd", i);
            SMCReadKey(key, &val);

            if (getFloatFromVal(val))
            {
                printf("    Mode         : forced\n");
            }
            else
            {
                printf("    Mode         : auto\n");
            }
        }
    }

    return kIOReturnSuccess;
}

double ReadMaxCPUTemperature(void)
{
    kern_return_t result;
    UInt32Char_t key = {0};
    SMCVal_t val;

    sprintf(key, "TCMX");

    result = SMCReadKey(key, &val);

    double maxCPUTemperature = 0;

    if (result != kIOReturnSuccess)
    {
        printf("Error: SMCReadKey() = %08x\n", result);
    }
    else if (val.dataSize > 0)
    {
        maxCPUTemperature = ((SInt16)ntohs(*(UInt16 *)val.bytes)) / 256.0;
    }


    return maxCPUTemperature;
}

int writeValue(char *key, char *in_value)
{

    SMCVal_t      val;
    kern_return_t result;


    int i;
    char c[3];

    for (i = 0; i < strlen(in_value); i++)
    {
        sprintf(c, "%c%c", in_value[i * 2], in_value[(i * 2) + 1]);
        val.bytes[i] = (int) strtol(c, NULL, 16);
    }

    val.dataSize = i / 2;

    if ((val.dataSize * 2) != strlen(in_value))
    {
        printf("Error: value is not valid\n");
        return 1;
    }

    if (strlen(key) > 0)
    {
        sprintf(val.key, "%s", key);
        result = SMCWriteKey(val);

        if (result != kIOReturnSuccess)
        {
            printf("Error: SMCWriteKey() = %08x\n", result);
            return 1;
        }
    }
    else
    {
        printf("Error: specify a key to write\n");
        return 1;
    }
    return 0;
}

void setFanSpeed(int fanIndex, double targetSpeed)
{
    SMCVal_t      val;
    kern_return_t result;

    char key[5] = {"\0"};
    sprintf(key, "F%dMd", fanIndex);

    writeValue(key, "01");

    float speed = (float)targetSpeed;

    for (int i = 0; i < 32; i++)
    {
        val.bytes[i] = 0;
    }

    sprintf(val.key, "F%dTg", fanIndex);
    memcpy(val.bytes, &speed, sizeof(float));
    val.dataSize = 4;
    result = SMCWriteKey(val);

    if (result != kIOReturnSuccess)
    {
        printf("Error: SMCWriteKey() = %08x\n", result);
    }
}

void writeFanValue(char *key, float speed)
{

    SMCVal_t      val;
    kern_return_t result;

    float speed2 = speed;

    for (int i = 0; i < 32; i++)
    {
        val.bytes[i] = 0;
    }


    if (strlen(key) > 0)
    {
        sprintf(val.key, "%s", key);
        memcpy(val.bytes, &speed2, sizeof(float));
        val.dataSize = 4;
        result = SMCWriteKey(val);

        if (result != kIOReturnSuccess)
        {
            printf("Error: SMCWriteKey() = %08x\n", result);
        }
    }
    else
    {
        printf("Error: specify a key to write\n");
    }

}

void SetFanSpeedByPercentage(double percentage)
{
    //!!!!!!!ONLY APPLIES TO DOUBLE FAN MACHINES!!!!!!
    SMCVal_t      val;
    SMCReadKey("F0Mn", &val);
    double fan0MinSpeed = getFloatFromVal(val);
    #ifdef DEBUG
    printf("fan0mn %f\n", fan0MinSpeed);
    #endif
    SMCReadKey("F1Mn", &val);
    double fan1MinSpeed = getFloatFromVal(val);
    #ifdef DEBUG
    printf("fan1mn %f\n", fan1MinSpeed);
    #endif
    SMCReadKey("F0Mx", &val);
    double fan0MaxSpeed = getFloatFromVal(val);
    #ifdef DEBUG
    printf("fan0mx %f\n", fan0MaxSpeed);
    #endif
    SMCReadKey("F1Mx", &val);
    double fan1MaxSpeed = getFloatFromVal(val);
    #ifdef DEBUG
    printf("fan1mx %f\n", fan1MaxSpeed);
    #endif


    int fan0TargetSpeed = (fan0MaxSpeed - fan0MinSpeed) * percentage + fan0MinSpeed;

    int fan1TargetSpeed = (fan1MaxSpeed - fan1MinSpeed) * percentage + fan1MinSpeed;

    if (fan0TargetSpeed > fan0MaxSpeed)
    {
        fan0TargetSpeed = fan0MaxSpeed;
    }

    if (fan0TargetSpeed < fan0MinSpeed)
    {
        fan0TargetSpeed = fan0MinSpeed;
    }

    if (fan1TargetSpeed > fan1MaxSpeed)
    {
        fan1TargetSpeed = fan1MaxSpeed;
    }

    if (fan1TargetSpeed < fan1MinSpeed)
    {
        fan1TargetSpeed = fan1MinSpeed;
    }

    setFanSpeed(0, fan0TargetSpeed);
    setFanSpeed(1, fan1TargetSpeed);
    return;
}

// sudo ./smc 61 10 30 2  66 30 60 4  71 60 95 6  74 16
void printUsage()
{
    fprintf(stderr,
    "DESCRIPTION:\n"
    "    This utility enables you to control your Mac's fans manually.\n"
    "    Note: This utility only applies to Intel Macs with 2 fans.\n"
    "    !!!You should execute this utility with root privileges!!!\n"
    "SYNOPSIS:\n"
    "    sfc_manual [-a] | [-A] | [--SMC-enhanced] | [-d] | [-h] | [-i]\n"
    "               [-m speed_percentage] | [-m speed_left speed_right]\n"
    "OPTIONS:\n"
    "    -a: Auto mode (controlled by SMC).\n"
    "    -A: Auto mode (controlled by this program)\n"
    "    --SMC-enhanced: Auto mode (an enhanced fan curve using SMC).\n"
    "    -d: turn off fans completely.\n"
    "        Note: This could easily cause your machine to overheat!!!\n"
    "    -h: display this message.\n"
    "    -i: show fan information.\n"
    "    -m <percentage>: set fan speeds to a specific percentage manually.\n"
    "    -m <speed_left> <speed_right>: set fan speeds to specific speeds manually.\n"
    "        Note: If you set fan speeds by RPM, it ignores Apple's limits.\n"
    "              You can \"overclock\" or \"underclock\" your fans,\n"
    "              but ridiculous values may damage you machine!!!\n"
    "EXAMPLES:\n"
    "    sfc_manual -m 50        // set both fans to 50 percent\n"
    "    sfc_manual -m 1080 1000 // left: 1080rpm; right: 1000rpm\n"
    "    sfc_manual -a           // set fans to auto mode (SMC)\n");
}

void setFanSpeedAccordingToTemperature(double temperature)
{
    static const double fan0MinRPM = 1481.75;
    static const double fan1MinRPM = 1372.25;
    static const double fan0MaxRPM = 5927.0;
    static const double fan1MaxRPM = 5489.0;
    static const double fan1ToFan0Ratio = fan1MaxRPM / fan0MaxRPM;

    // static const double temperatureToMinRPM = 58.0;
    // static const double temperatureToMaxRPM = 80.0;
    static const double temperatureWhenFansStopSpinning  = 54.0;
    static const double temperatureWhenFansStartSpinning = 58.0;

    static bool areFansOn = true;
    double fan0TargetRPM = 0;
    double fan1TargetRPM = 0;

    // static const double fan0TemperatureToSpeedCoefficient_a = (fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    // static const double fan0TemperatureToSpeedCoefficient_b = -2 * temperatureToMinRPM * (fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    // static const double fan0TemperatureToSpeedCoefficient_c = (fan0MaxRPM * temperatureToMinRPM * temperatureToMinRPM + fan0MinRPM * temperatureToMaxRPM * temperatureToMaxRPM - 2 * fan0MinRPM * temperatureToMaxRPM * temperatureToMinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    // static const double fan1TemperatureToSpeedCoefficient_a = (fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    // static const double fan1TemperatureToSpeedCoefficient_b = -2 * temperatureToMinRPM * (fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    // static const double fan1TemperatureToSpeedCoefficient_c = (fan1MaxRPM * temperatureToMinRPM * temperatureToMinRPM + fan1MinRPM * temperatureToMaxRPM * temperatureToMaxRPM - 2 * fan1MinRPM * temperatureToMaxRPM * temperatureToMinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);

    // // printf("%.1f %.1f %.1f", fan0TemperatureToSpeedCoefficient_a, fan0TemperatureToSpeedCoefficient_b, fan0TemperatureToSpeedCoefficient_c);


    // if (temperature >= temperatureToMinRPM)
    // {
    //     // y = 9.184400826 x^2 - 1065.390496 x + 32378.07438
    //     fan0TargetRPM = fan0TemperatureToSpeedCoefficient_a * temperature * temperature + fan0TemperatureToSpeedCoefficient_b * temperature + fan0TemperatureToSpeedCoefficient_c;
    //     fan1TargetRPM = fan1TemperatureToSpeedCoefficient_a * temperature * temperature + fan1TemperatureToSpeedCoefficient_b * temperature + fan1TemperatureToSpeedCoefficient_c;
    // }
    // else
    // {
    //     // y = 202.0568182 x-10237.54546
    //     fan0TargetRPM = ((fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperature + fan0MaxRPM - ((fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperatureToMaxRPM;
    //     fan1TargetRPM = ((fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperature + fan1MaxRPM - ((fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperatureToMaxRPM;
    // }

    if (temperature <= 65.0)
    {
        fan0TargetRPM = 163.65 * temperature - 8337.25;
    }
    else
    {
        fan0TargetRPM = 241.8 * temperature - 13417.0;
    }

    fan1TargetRPM = fan0TargetRPM * fan1ToFan0Ratio;



    if (fan0TargetRPM < fan0MinRPM)
    {
        if (temperature < temperatureWhenFansStopSpinning)
        {
            fan0TargetRPM = 0;
        }
        else
        {
            fan0TargetRPM = fan0MinRPM;
        }
    }

    if ((!areFansOn)
        && temperature < temperatureWhenFansStartSpinning)
    {
        fan0TargetRPM = 0;
    }



    if (fan1TargetRPM < fan1MinRPM)
    {
        if (temperature < temperatureWhenFansStopSpinning)
        {
            fan1TargetRPM = 0;
        }
        else
        {
            fan1TargetRPM = fan1MinRPM;
        }

    }

    if ((!areFansOn)
        && temperature < temperatureWhenFansStartSpinning)
    {
        fan1TargetRPM = 0;
    }


    if (fan0TargetRPM > fan0MaxRPM)
    {
        fan0TargetRPM = fan0MaxRPM;
    }

    if (fan1TargetRPM > fan1MaxRPM)
    {
        fan1TargetRPM = fan1MaxRPM;
    }


    #ifdef DEBUG
    printf("tg: %.0f %.0f", fan0TargetRPM, fan1TargetRPM);
    #endif

    if (fan0TargetRPM < 10)
    {
        areFansOn = false;
    }
    else
    {
        areFansOn = true;
    }

    if (fan1TargetRPM < 10)
    {
        areFansOn = false;
    }
    else
    {
        areFansOn = true;
    }

    setFanSpeed(0, fan0TargetRPM);
    setFanSpeed(1, fan1TargetRPM);
}

void exit_failure()
{
    smc_close();
    exit(EXIT_FAILURE);
}

void exit_success()
{
    smc_close();
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[])
{


    if (!(argc == 2 || argc == 3 || argc == 4))
    {
        puts("Incorrect number of parameters.");
        puts("Use option \"-h\" for help.");
        // printUsage();
        exit_failure();
    }

    smc_init();

    if (!strcmp(argv[1], "-m"))
    {
        if (!(argc == 3 || argc == 4))
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else if (argc == 3)
        {
            if (strspn(argv[2], "0123456789") == strlen(argv[2]))
            {
                double percentage = atoi(argv[2]) / 100.0;
                SetFanSpeedByPercentage(percentage);
            }
            else
            {
                puts("The parameter should only be integers.");
                puts("Use option \"-h\" for help.");
                // printUsage();
                exit_failure();
            }

            exit_success();
        }
        else
        {
            double fan0spd = 0.0;
            double fan1spd = 0.0;

            if (strspn(argv[2], "0123456789") == strlen(argv[2]))
            {
                fan0spd = (double)atoi(argv[2]);
            }
            else
            {
                puts("The parameter should only be integers.");
                puts("Use option \"-h\" for help.");
                // printUsage();
                exit_failure();
            }

            if (strspn(argv[3], "0123456789") == strlen(argv[3]))
            {
                fan1spd = (double)atoi(argv[3]);
            }
            else
            {
                puts("The parameter should only be integers.");
                puts("Use option \"-h\" for help.");
                // printUsage();
                exit_failure();
            }

            setFanSpeed(0, fan0spd);
            setFanSpeed(1, fan1spd);
            exit_success();
        }
    }
    else if (!strcmp(argv[1], "-a"))
    {
        if (argc != 2)
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else
        {
            writeValue("F0Md", "00");
            writeValue("F1Md", "00");
            exit_success();
        }
    }
    else if (!strcmp(argv[1], "-d"))
    {
        if (argc != 2)
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else
        {
            setFanSpeed(0, 0.0);
            setFanSpeed(1, 0.0);
            exit_success();
        }
    }
    else if (!strcmp(argv[1], "-i"))
    {
        if (argc != 2)
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else
        {

            kern_return_t result = SMCPrintFans();

            if (result != kIOReturnSuccess)
            {
                printf("Error: SMCPrintFans() = %08x\n", result);
            }

            exit_success();
        }
    }
    else if (!strcmp(argv[1], "-A"))
    {
        if (argc != 2)
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else
        {

            #ifdef DAEMON
            smc_close();
            daemonize();
            smc_init();
            #endif
            const size_t CPU_TEMP_LOG_DURATION = 90;
            size_t idxCPUTempHistory = 0;
            double CPUTempHistory[CPU_TEMP_LOG_DURATION] = {0};

            sleep(2);
            double CPUTemperatureNow = ReadMaxCPUTemperature();

            for (size_t i = 0; i < CPU_TEMP_LOG_DURATION; i++)
            {
                CPUTempHistory[i] = CPUTemperatureNow;
            }

            for (;;)
            {
                if (idxCPUTempHistory >= CPU_TEMP_LOG_DURATION)
                {
                    idxCPUTempHistory = 0;
                }

                CPUTempHistory[idxCPUTempHistory] = ReadMaxCPUTemperature();
                #ifdef DEBUG
                printf("cur: %.1f | ", CPUTempHistory[idxCPUTempHistory]);
                #endif
                idxCPUTempHistory++;

                double sumCPUTempHistory = 0;

                for (size_t i = 0; i < CPU_TEMP_LOG_DURATION; i++)
                {
                    sumCPUTempHistory += CPUTempHistory[i];
                }

                double avgCPUTemp = sumCPUTempHistory / (double)CPU_TEMP_LOG_DURATION;
                #ifdef DEBUG
                printf("avg: %.2f | ", avgCPUTemp);
                #endif

                setFanSpeedAccordingToTemperature(avgCPUTemp);

                #ifdef DEBUG
                printf("\n");
                fflush(stdout);
                #endif

                sleep(1);
            }


        }
    }
    else if (!strcmp(argv[1], "--SMC-enhanced"))
    {
        if (argc != 2)
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else
        {

            #ifdef DAEMON
            smc_close();
            daemonize();
            smc_init();
            #endif
            const double fan0MinSpeed = getFloatFromKey("F0Mn");
            const size_t CPU_TEMP_LOG_DURATION = 90;
            size_t idxCPUTempHistory = 0;
            double CPUTempHistory[CPU_TEMP_LOG_DURATION] = {0.0};
            bool areFansOn = true;

            sleep(2);
            double CPUTemperatureNow = ReadMaxCPUTemperature();

            for (size_t i = 0; i < CPU_TEMP_LOG_DURATION; i++)
            {
                CPUTempHistory[i] = CPUTemperatureNow;
            }


            for (;;)
            {
                if (idxCPUTempHistory >= CPU_TEMP_LOG_DURATION)
                {
                    idxCPUTempHistory = 0;
                }

                CPUTempHistory[idxCPUTempHistory] = ReadMaxCPUTemperature();
                #ifdef DEBUG
                printf("cur: %.1f | ", CPUTempHistory[idxCPUTempHistory]);
                #endif
                idxCPUTempHistory++;

                double sumCPUTempHistory = 0.0;
                double fan0TargetSpeed = 0.0;
                bool isFan0LowRPM = true;

                if (getFloatFromKey("F0Ac") > fan0MinSpeed + 40)
                {
                    isFan0LowRPM = false;
                }

                for (size_t i = 0; i < CPU_TEMP_LOG_DURATION; i++)
                {
                    sumCPUTempHistory += CPUTempHistory[i];
                }

                double avgCPUTemp = sumCPUTempHistory / (double)CPU_TEMP_LOG_DURATION;
                #ifdef DEBUG
                printf("avg: %.2f | ", avgCPUTemp);
                printf("isfan0Low: %d | ", isFan0LowRPM);
                #endif

                if (isFan0LowRPM && avgCPUTemp < 62)
                {
                    if (areFansOn)
                    {
                        if (avgCPUTemp > 57.30091)
                        {
                            fan0TargetSpeed = 169.5625 * avgCPUTemp - 8352.875;
                        }
                        else if (avgCPUTemp < 54)
                        {
                            fan0TargetSpeed = 0.0;
                        }
                        else
                        {
                            fan0TargetSpeed = 1363.21;
                        }
                    }
                    else
                    {
                        if (avgCPUTemp > 57.30091)
                        {
                            fan0TargetSpeed = 169.5625 * avgCPUTemp - 8352.875;
                        }
                        else
                        {
                            fan0TargetSpeed = 0;
                        }
                    }

                    #ifdef DEBUG
                    printf("fan0Tg: %.0f | ", fan0TargetSpeed);
                    #endif

                    setFanSpeed(0, fan0TargetSpeed);
                    setFanSpeed(1, fan0TargetSpeed * 5489.0 / 5927.0);

                    if (fan0TargetSpeed < 10)
                    {
                        areFansOn = false;
                    }
                    else
                    {
                        areFansOn = true;
                    }
                }
                else
                {
                    if (getFloatFromKey("F0Md"))
                    {
                        // if fans are currently forced, set them to auto
                        writeValue("F0Md", "00");
                        writeValue("F1Md", "00");
                        areFansOn = true;
                        #ifdef DEBUG
                        printf("hasSetToAuto");
                        #endif
                    }
                }



                #ifdef DEBUG
                printf("\n");
                fflush(stdout);
                #endif

                sleep(1);
            }


        }
    }
    else if (!strcmp(argv[1], "-h"))
    {
        if (argc != 2)
        {
            puts("Incorrect parameters.");
            puts("Use option \"-h\" for help.");
            // printUsage();
            exit_failure();
        }
        else
        {
            printUsage();

            exit_success();
        }
    }
    else
    {
        puts("Incorrect parameters.");
        puts("Use option \"-h\" for help.");
        // printUsage();
        exit_failure();
    }

    smc_close();
    return EXIT_SUCCESS;
}

//TODO:
//encapculate into classes
//split files
//optimize code
