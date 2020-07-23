#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>
#include <IOKit/IOKitLib.h>
#include "smc.h"

// TODO 
// when error occurs, remember smc_close()

#define DEBUG
#define DAEMON

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

    // close(STDIN_FILENO);
    // close(STDOUT_FILENO);
    // close(STDERR_FILENO);
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
    int c;
    kern_return_t result;
    UInt32Char_t key = {0};
    SMCVal_t val;

    sprintf(key, "TCMX");

    result = SMCReadKey(key, &val);

    double maxCPUTemperature;

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
        val.bytes[i] = (int) strtol(c, nullptr, 16);
    }

    val.dataSize = i / 2;

    if ((val.dataSize * 2) != strlen(in_value))
    {
        printf("Error: value is not valid\n");
        return 1;
    }

    if (strlen(key) > 0)
    {
        sprintf(val.key, key);
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

void setFanSpeed(size_t fanIndex, double targetSpeed)
{
    SMCVal_t      val;
    kern_return_t result;

    char key[5] = {"\0"};
    sprintf(key, "F%dMd", fanIndex);

    writeValue(key, "01");

    float speed = static_cast<float>(targetSpeed);

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
        sprintf(val.key, key);
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

    writeValue("F0Md", "01");
    writeValue("F1Md", "01");
    writeFanValue("F0Tg", (float)fan0TargetSpeed);
    writeFanValue("F1Tg", (float)fan1TargetSpeed);
    return;
}

// sudo ./smc 61 10 30 2  66 30 60 4  71 60 95 6  74 16
void printUsage()
{
    puts("DESCRIPTION:");
    puts("    This utility enables you to control your Mac's fans manually.");
    puts("    Note: This utility only applies to Intel Macs with 2 fans.");
    puts("    You should execute this utility with root privileges.");
    puts("SYNOPSIS:");
    puts("    sfc_manual [-m speed | -a | -d]");
    puts("OPTIONS:");
    puts("    -a: set fans to auto mode (controlled by SMC).");
    puts("    -d: disable fans.");
    puts("    -i: show fan information.");
    puts("    -m: set fan speed to a specific percentage or speed manually. (only input integers)");
    puts("        -m <percentage> OR");
    puts("        -m <left fan RPM> <right fan RPM>");
    puts("        Note: If you set fan speeds by RPM, it ignores Apple's limits.");
    puts("              You can \"overclock\" or \"underclock\" your fans,");
    puts("              but ridiculous values may damage you machine!!!");
    puts("EXAMPLES:");
    puts("    sfc_manual -m 50        // set both fans to 50 percent");
    puts("    sfc_manual -m 1080 1000 // left: 1080rpm; right: 1000rpm");
    puts("    sfc_manual -a           // set fans to auto mode");
}



void setFanSpeedAccordingToTemperature(double temperature)
{
    static const double fan0MinRPM = 1481.75;
    static const double fan1MinRPM = 1372.25;
    static const double fan0MaxRPM = 5927.0;
    static const double fan1MaxRPM = 5489.0;
    static const double fan0RPMStep = (fan0MaxRPM - fan0MinRPM) / 100;
    static const double fan1RPMStep = (fan1MaxRPM - fan1MinRPM) / 100;

    static const double temperatureToMinRPM = 60.0;
    static const double temperatureToMaxRPM = 80.0;

    static const double fan0TemperatureToSpeedCoefficient_a = (fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    static const double fan0TemperatureToSpeedCoefficient_b = -2 * temperatureToMinRPM * (fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    static const double fan0TemperatureToSpeedCoefficient_c = (fan0MaxRPM * temperatureToMinRPM * temperatureToMinRPM + fan0MinRPM * temperatureToMaxRPM * temperatureToMaxRPM - 2 * fan0MinRPM * temperatureToMaxRPM * temperatureToMinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    static const double fan1TemperatureToSpeedCoefficient_a = (fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    static const double fan1TemperatureToSpeedCoefficient_b = -2 * temperatureToMinRPM * (fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);
    static const double fan1TemperatureToSpeedCoefficient_c = (fan1MaxRPM * temperatureToMinRPM * temperatureToMinRPM + fan1MinRPM * temperatureToMaxRPM * temperatureToMaxRPM - 2 * fan1MinRPM * temperatureToMaxRPM * temperatureToMinRPM) / (temperatureToMaxRPM * temperatureToMaxRPM - 2 * temperatureToMaxRPM * temperatureToMinRPM + temperatureToMinRPM * temperatureToMinRPM);

    //printf("%.1f %.1f %.1f", fan0TemperatureToSpeedCoefficient_a, fan0TemperatureToSpeedCoefficient_b, fan0TemperatureToSpeedCoefficient_c);
    static bool areFansOn = true;
    double fan0TargetRPM = 0;
    double fan1TargetRPM = 0;

    if (temperature >= temperatureToMinRPM)
    {
        fan0TargetRPM = fan0TemperatureToSpeedCoefficient_a * temperature * temperature + fan0TemperatureToSpeedCoefficient_b * temperature + fan0TemperatureToSpeedCoefficient_c;
        fan1TargetRPM = fan1TemperatureToSpeedCoefficient_a * temperature * temperature + fan1TemperatureToSpeedCoefficient_b * temperature + fan1TemperatureToSpeedCoefficient_c;
    }
    else
    {
        fan0TargetRPM = ((fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperature + fan0MaxRPM - ((fan0MaxRPM - fan0MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperatureToMaxRPM;
        fan1TargetRPM = ((fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperature + fan1MaxRPM - ((fan1MaxRPM - fan1MinRPM) / (temperatureToMaxRPM - temperatureToMinRPM)) * temperatureToMaxRPM;
    }



    if (fan0TargetRPM < fan0MinRPM)
    {
        if (temperature < temperatureToMinRPM - 3)
        {
            fan0TargetRPM = 0;
        }
        else
        {
            fan0TargetRPM = fan0MinRPM;
        }
    }

    if ((!areFansOn)
        && temperature < temperatureToMinRPM + 3)
    {
        fan0TargetRPM = 0;
    }



    if (fan1TargetRPM < fan1MinRPM)
    {
        if (temperature < temperatureToMinRPM - 3)
        {
            fan1TargetRPM = 0;
        }
        else
        {
            fan1TargetRPM = fan1MinRPM;
        }

    }

    if ((!areFansOn)
        && temperature < temperatureToMinRPM + 3)
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
    printf("tg: %.1f %.1f, ", fan0TargetRPM, fan1TargetRPM);
    #endif

    // SMCVal_t      val;
    // SMCReadKey("F0Ac", &val);
    // double fan0ActualSpeed = getFloatFromVal(val);
    // SMCVal_t      val;
    // SMCReadKey("F1Ac", &val);
    // double fan1ActualSpeed = getFloatFromVal(val);

    // if (fabs(fan0TargetRPM - fan0ActualSpeed) > 80)
    // {
    //     if (fan0TargetRPM > fan0ActualSpeed)
    //     {
    //         setFanSpeed(0, fan0ActualSpeed + fan0RPMStep);
    //     }
    //     else
    //     {
    //         setFanSpeed(0, fan0ActualSpeed - fan0RPMStep);
    //     }
    // }

    // if (fabs(fan1TargetRPM - fan1ActualSpeed) > 80)
    // {
    //     if (fan1TargetRPM > fan1ActualSpeed)
    //     {
    //         setFanSpeed(1, fan1ActualSpeed + fan1RPMStep);
    //     }
    //     else
    //     {
    //         setFanSpeed(1, fan1ActualSpeed - fan1RPMStep);
    //     }
    // }

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
int main(int argc, char *argv[])
{


    if (!(argc == 2 || argc == 3 || argc == 4))
    {
        puts("incorrect number of parameters.");
        printUsage();
        return 1;
    }

    smc_init();

    if (!strcmp(argv[1], "-m"))
    {
        if (!(argc == 3 || argc == 4))
        {
            puts("incorrect parameters.");
            printUsage();
            return 1;
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
                puts("the parameter should be only numbers");
                printUsage();
                return 1;
            }

            return 0;
        }
        else
        {
            float fan0spd;
            float fan1spd;

            if (strspn(argv[2], "0123456789") == strlen(argv[2]))
            {
                fan0spd = (float)atoi(argv[2]);
            }
            else
            {
                puts("the parameter should only be numbers.");
                printUsage();
                return 1;
            }

            if (strspn(argv[3], "0123456789") == strlen(argv[3]))
            {
                fan1spd = (float)atoi(argv[3]);
            }
            else
            {
                puts("the parameter should only be numbers.");
                printUsage();
                return 1;
            }

            writeValue("F0Md", "01");
            writeValue("F1Md", "01");
            writeFanValue("F0Tg", fan0spd);
            writeFanValue("F1Tg", fan1spd);
            return 0;
        }
    }
    else if (!strcmp(argv[1], "-a"))
    {
        if (argc != 2)
        {
            puts("incorrect parameters.");
            printUsage();
            return 1;
        }
        else
        {
            writeValue("F0Md", "00");
            writeValue("F1Md", "00");
            return 0;
        }
    }
    else if (!strcmp(argv[1], "-d"))
    {
        if (argc != 2)
        {
            puts("incorrect parameters.");
            printUsage();
            return 1;
        }
        else
        {
            writeValue("F0Md", "01");
            writeValue("F1Md", "01");
            writeFanValue("F0Tg", (float)0);
            writeFanValue("F1Tg", (float)0);
            return 0;
        }
    }
    else if (!strcmp(argv[1], "-i"))
    {
        if (argc != 2)
        {
            puts("incorrect parameters.");
            printUsage();
            return 1;
        }
        else
        {

            kern_return_t result = SMCPrintFans();

            if (result != kIOReturnSuccess)
            {
                printf("Error: SMCPrintFans() = %08x\n", result);
            }

            return 0;
        }
    }
    else if (!strcmp(argv[1], "-A"))
    {
        if (argc != 2)
        {
            puts("incorrect parameters.");
            printUsage();
            return 1;
        }
        else
        {
            
            #ifdef DAEMON
            smc_close();
            daemonize();
            smc_init();
            #endif
            const size_t CPU_TEMP_LOG_DURATION = 60;
            size_t idxCPUTempHistory = 0;
            double CPUTempHistory[CPU_TEMP_LOG_DURATION] = {0};


            for (;;)
            {
                if (idxCPUTempHistory >= CPU_TEMP_LOG_DURATION)
                {
                    idxCPUTempHistory = 0;
                }

                CPUTempHistory[idxCPUTempHistory] = ReadMaxCPUTemperature();
                #ifdef DEBUG
                printf("cur: %.1f, ", CPUTempHistory[idxCPUTempHistory]);
                #endif
                idxCPUTempHistory++;

                double sumCPUTempHistory = 0;

                for (size_t i = 0; i < CPU_TEMP_LOG_DURATION; i++)
                {
                    sumCPUTempHistory += CPUTempHistory[i];
                }

                double avgCPUTemp = sumCPUTempHistory / (double)CPU_TEMP_LOG_DURATION;
                #ifdef DEBUG
                printf("avg: %.1f, ", avgCPUTemp);
                #endif

                setFanSpeedAccordingToTemperature(avgCPUTemp);

                #ifdef DEBUG
                printf("\n");
                fflush(stdout);
                #endif

                sleep(1);
            }


            return 0;
        }
    }
    else
    {
        puts("incorrect parameters.");
        printUsage();
        return 1;
    }


    smc_close();
}
#pragma clang diagnostic pop


//TODO:
//encapculate into classes
//split files
//optimize code



