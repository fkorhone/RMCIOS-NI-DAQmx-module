/*
RMCIOS - Reactive Multipurpose Control Input Output System
Copyright (c) 2018 Frans Korhonen

RMIOS was originally developed at Institute for Atmospheric 
and Earth System Research / Physics, Faculty of Science, 
University of Helsinki, Finland

Assistance, experience and feedback from following persons have been 
critical for development of RMCIOS: Erkki Siivola, Juha Kangasluoma, 
Lauri Ahonen, Ella Häkkinen, Pasi Aalto, Joonas Enroth, Runlong Cai, 
Markku Kulmala and Tuukka Petäjä.

This file is extension to RMCIOS. This notice was encoded using utf-8.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/. 
*/

/*
 * Changelog: (date,who,description)
 */

#define DLL

#include <inttypes.h>
#include <NIDAQmx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "RMCIOS-functions.h"

///////////////////////////////////////////////////
// Analog input
///////////////////////////////////////////////////
#define DAQmxErrChk(functionCall) if( DAQmxFailed(functionCall) ){ \
                                    char errBuff[2048] ; errBuff[0]=0 ; \
                                    DAQmxGetExtendedErrorInfo(errBuff,2048) ; \
                                    printf("%s\r\n",errBuff) ;}

struct ni_device_data
{
   int channel_id;
   char name[20];
   TaskHandle task;
   int channels;
   int samples;
   float64 rate;
   float values[100];

   // linked list of ni devices in the system
   struct ni_device_data *next_device;  
} *first_ni_device = NULL; // pointer to first ni device in the system.

// Helper function to find ni_device data for given NI device channel id.
struct ni_device_data *get_ni_device_for_channel (int channel_id)
{
   // Find the specified device:
   struct ni_device_data *device = first_ni_device;
   while (device != NULL)
   {
      if (device->channel_id == channel_id)
         return device;
      device = device->next_device;
   }
}

void ni_device_func (struct ni_device_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv,
                     int num_params, const union param_rmcios param)
{
   int i;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for niai device. Commands:\r\n"
                     "create nidev newname \r\n"
                     "setup newname device_name | sample_rate | samples \r\n"
                     "write newname do one measurement\r\n");

   case create_rmcios:
      if (num_params < 1)
         break;

      // allocate new data
      this = (struct ni_device_data *) malloc (sizeof (struct ni_device_data)); 
      if (this == NULL) break;
      
      // create channel     
      this->channel_id = create_channel_param (context, paramtype, param, 0, 
                                               (class_rmcios) ni_device_func, 
                                               this); 

      //default values:
      this->name[0] = 0;
      this->task = 0;
      this->channels = 0;
      this->samples = 1;
      this->rate = 10;
      this->next_device = NULL;

      //add device to list of NIDAQ devices:
      if (first_ni_device == NULL)
      {
         // this is the first device in the system
         first_ni_device = this;  
      }
      else
      {
         // find the last device
         struct ni_device_data *devices = first_ni_device;
         while (devices->next_device != NULL)
            devices = devices->next_device;     
         
         // Add this device to the list
         devices->next_device = this;   
      }

      // Create the device task
      DAQmxErrChk (DAQmxCreateTask ("", //const char taskName[], 
                                    &this->task)); //TaskHandle *taskHandle);
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      
      // device name
      param_to_string (context, paramtype, param, 0, 
                       sizeof (this->name), this->name);  
      
      if (num_params >= 3)
         // 3.samples
         this->samples = param_to_int (context, paramtype, param, 2); 
      if (num_params >= 2)
         // 2.sample_rate
         this->rate = param_to_int (context, paramtype, param, 1);      
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      {
         int32 read = 0;
         int ch;
         int i;
         float freturn;
         float64 buffer[this->samples * this->channels];
         float values[this->channels];

         if (this->task != 0)
         {
            DAQmxStopTask (this->task);
         }
         // Create the device task
         DAQmxErrChk (DAQmxStartTask (this->task));

         // (TaskHandle taskHandle, 
         DAQmxErrChk (
            DAQmxReadAnalogF64 (this->task,   
                                DAQmx_Val_Auto, // int32 numSampsPerChan, 
                                10,   // float64 timeout, 
                                DAQmx_Val_GroupByChannel, // bool32 fillMode
                                buffer,       // float64 readArray[],
                                this->samples * this->channels, 
                                &read,        // int32 *sampsPerChanRead,
                                NULL));       // bool32 *reserved);

         if (read != this->samples)
         {
            printf ("ERROR DAQMX wrong ammount of samples read: %d\r\n", read);
         }
         else
         {
            for (ch = 0; ch < this->channels; ch++) // average loop
            {
               this->values[ch] = 0;
               float64 *ch_data = buffer + ch * this->samples;

               for (i = 0; i < this->samples; i++)
               {
                  this->values[ch] += ch_data[i];

               }
               this->values[ch] /= this->samples;
            }
            //int channel,
            context->run_channel (context, linked_channels (context, id), 
                                  write_rmcios, 
                                  float_rmcios, 
                                  (union param_rmcios) &freturn, 
                                  this->channels,       
                                  (const union param_rmcios) this->values); 
         }
      }
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      {
         int i;
         for (i = 0; i < this->channels; i++)
         {
            return_float (context, paramtype, returnv, this->values[i]);
            return_string (context, paramtype, returnv, " ");
         }
      }
      break;
   }
}

struct niai_data
{
   int id;
   int channel_index;
   float value;
};

void nidaq_ai_func (struct niai_data *this,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    union param_rmcios returnv,
                    int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for niai - NI analog input\r\n"
                     " create niai newname\r\n"
                     " setup newname ni_device_channel | terminal\r\n"
                     "               | term_cfg(RSE NRSE Diff PseudoDiff) \r\n"
                     "               | minVal maxVal\r\n"
                     " read newname #read latest analog value \r\n"
                     " link newname linked_ch #link output to channel \r\n");
      break;
   case create_rmcios: 
      // params: channel_name=0 device_channel=1 
      // term_cfg(RSE NRSE Diff PseudoDiff)=2 minVal=3 maxVal=4
      if (num_params < 1)
      {
         printf ("Not enough parameters\r\n");
         break;
      }
      // allocate new data
      this = (struct niai_data *) malloc (sizeof (struct niai_data));   
      if (this == NULL) break;
      
      // create channel
      this->id = create_channel_param (context, paramtype, param, 0, 
                                       (class_rmcios) nidaq_ai_func, this);
      
      // Default values: 
      this->channel_index = 0;
      this->value = 0;
      break;

   case setup_rmcios:
      if (this == NULL) break;
      if (num_params < 2)
         break;
      {
         int i;
         char term_str[30], term_cfg_str[15];

         // Get the NI device for given channel:
         struct ni_device_data *device =
            get_ni_device_for_channel (param_to_int
                                       (context, paramtype, param, 0));
         if (device == NULL)
         {
            printf ("NO NI device channel: %s\r\n",
                    param_to_string (context, paramtype, param, 1, 0, NULL));
            break;
         }
         
         // link NIAI device analog output data to this channel.
         link_channel (context, param_to_int (context, paramtype, param, 0), 
                       this->id); 

         // Build physical channel string:
         param_to_string (context, paramtype, param, 1,
                          sizeof (term_str), term_str);

         char physicalChannel[strlen (term_str) + strlen (device->name) + 3];
         strcpy (physicalChannel, device->name);
         strcat (physicalChannel, "/");
         strcat (physicalChannel, term_str);

         int term_cfg = DAQmx_Val_Cfg_Default;
         if (num_params >= 3)
         {
            param_to_string (context, paramtype, param, 2,
                             sizeof (term_cfg_str), term_cfg_str);
            if (strcmp (term_cfg_str, "RSE") == 0)
               term_cfg = DAQmx_Val_RSE;
            if (strcmp (term_cfg_str, "NRSE") == 0)
               term_cfg = DAQmx_Val_NRSE;
            if (strcmp (term_cfg_str, "Diff") == 0)
               term_cfg = DAQmx_Val_Diff;
            if (strcmp (term_cfg_str, "PseudoDiff") == 0)
               term_cfg = DAQmx_Val_PseudoDiff;
         }
         float64 minVal = -10.0, maxVal = 10.0;

         if (num_params >= 5)
         {
            minVal = param_to_float (context, paramtype, param, 3);
            maxVal = param_to_float (context, paramtype, param, 4);
         }

         /////////////////////////////////////////
         // Configure the NI device:
         /////////////////////////////////////////

         if (device->task != 0)
         {
            DAQmxStopTask (device->task);
         }
         DAQmxErrChk (
            DAQmxCreateAIVoltageChan (device->task, // (TaskHandle taskHandle, 
                                      physicalChannel,  
                                      "", //nameToAssignToChannel[], 
                                      term_cfg, //int32 terminalConfig, 
                                      minVal, //float64 minVal, 
                                      maxVal, //float64 maxVal, 
                                      DAQmx_Val_Volts,  //int32 units, 
                                      ""));  //const char customScaleName[]);

         this->channel_index = device->channels;
         device->channels++;

         DAQmxErrChk (
            DAQmxCfgSampClkTiming (device->task,      //(TaskHandle taskHandle, 
                                   "",                //const char source[], 
                                   device->rate,      //float64 rate, 
                                   DAQmx_Val_Rising,  //int32 activeEdge, 
                                   DAQmx_Val_FiniteSamps, //int32 sampleMode, 
                                   device->samples)); // sampsPerChanToAcquire);

         DAQmxErrChk (DAQmxStartTask (device->task)); //(TaskHandle *taskHandle);
         return_int (context, paramtype, returnv, device->channels - 1);
      }

      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->value);
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params <= this->channel_index)
         break;
      this->value =
         param_to_float (context, paramtype, param, this->channel_index);
      write_f (context, linked_channels (context, id), this->value);
      break;
   }
}

///////////////////////////////////////////////////////////////////////////
// Analog output
///////////////////////////////////////////////////////////////////////////
struct niao_data
{
   TaskHandle task;
   float value;
   float64 minVal;
   float64 maxVal;
};

void nidaq_ao_func (struct niao_data *this,
                    const struct context_rmcios *context, int id,
                    enum function_rmcios function,
                    enum type_rmcios paramtype,
                    union param_rmcios returnv,
                    int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for niao.\r\n"
                     " create niao newname\r\n"
                     " setup newname device_channel terminal | minVal maxVal\r\n"
                     " write newname value\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;

      // allocate new data
      this = (struct niao_data *) malloc (sizeof (struct niao_data));   
      // create channel       
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) nidaq_ao_func, this);  

      //default values:
      this->task = 0;
      this->value = 0;
      this->minVal = -10.0;
      this->maxVal = 10.0;
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 2)
         break;
      if (num_params >= 4)
      {
         this->minVal = param_to_float (context, paramtype, param, 2);
         this->maxVal = param_to_float (context, paramtype, param, 3);
      }

      if (this->task != 0)
      {
         DAQmxErrChk (DAQmxStopTask (this->task));  //(TaskHandle taskHandle);
         DAQmxErrChk (DAQmxClearTask (this->task)); //(TaskHandle taskHandle);
         this->task = 0;
      }

      // Get the NI device for given channel:
      struct ni_device_data *device =
         get_ni_device_for_channel (param_to_int
                                    (context, paramtype, param, 0));
      if (device == NULL)
      {
         printf ("NO NI device channel: %s\r\n",
                 param_to_string (context, paramtype, param, 0, 0, NULL));
         break;
      }
      else
      {
         // Build physical channel string:
         char term_str[20];
         param_to_string (context, paramtype, param, 1,
                          sizeof (term_str), term_str);

         char physicalChannel[strlen (term_str) + strlen (device->name) + 3];
         strcpy (physicalChannel, device->name);
         strcat (physicalChannel, "/");
         strcat (physicalChannel, term_str);

         DAQmxErrChk (DAQmxCreateTask ("", //const char taskName[], 
                                       &this->task)); //TaskHandle *taskHandle);

         DAQmxErrChk (
               DAQmxCreateAOVoltageChan (this->task,//(TaskHandle taskHandle, 
                                         physicalChannel,  
                                         "", //nameToAssignToChannel[], 
                                         this->minVal, //float64 minVal, 
                                         this->maxVal, //float64 maxVal, 
                                         DAQmx_Val_Volts, //int32 units, 
                                         "")); //const char customScaleName[]);

         DAQmxErrChk (DAQmxStartTask (this->task));
      }
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      if (this->task == 0)
         break;

      this->value = param_to_float (context, paramtype, param, 0);

      DAQmxErrChk (DAQmxWriteAnalogScalarF64 (this->task, // (TaskHandle  
                                              0,   //bool32 autoStart, 
                                              0.5, //float64 timeout, 
                                              this->value,  //float64 value, 
                                              NULL));   //bool32 *reserved);

      write_f (context, linked_channels (context, id), this->value);
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->value);
      break;
   }
}

////////////////////////////////////////////////////////////////////
// PWM output
////////////////////////////////////////////////////////////////////
struct nipwm_data
{
   TaskHandle task;
   float64 duty;                // 
   float64 frequency;           // in hz
   int idle_state;              // 1 or 0 ;
};

void nipwm_func (struct nipwm_data *this,
                 const struct context_rmcios *context, int id,
                 enum function_rmcios function,
                 enum type_rmcios paramtype,
                 union param_rmcios returnv,
                 int num_params, const union param_rmcios param)
{
   int32 written;
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for nipwm\r\n"
                     "create nipwm ch_name\r\n"
                     "setup ch_name frequency device_channel counter_resource"
                     "              | idle_state\r\n" 
                     "   #Example: setup pwm1 1000 Dev1 ctr0 0\r\n"
                     "write ch_name duty_cycle\r\n"
                     "   #set duty and send applied duty to linked channels\r\n"
                     "read ch_name \r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // allocate new data
      this = (struct nipwm_data *) malloc (sizeof (struct nipwm_data)); 
      
      // create channel    
      create_channel_param (context, paramtype, param, 0, 
                            (class_rmcios) nipwm_func, this); 

      //default values:
      this->duty = 0.001;
      this->frequency = 1000;   // 1khz
      this->task = 0;
      this->idle_state = DAQmx_Val_Low;
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 3)
         break;

      this->frequency = param_to_float (context, paramtype, param, 0);

      // Get the NI device for given channel:
      struct ni_device_data *device =
         get_ni_device_for_channel (param_to_int
                                    (context, paramtype, param, 1));
      if (device == NULL)
      {
         printf ("No NI device channel: %s\r\n",
                 param_to_string (context, paramtype, param, 1, 0, NULL));
         break;
      }
      else
      {
         // Build physical channel string:
         char ctr_str[30];
         param_to_string (context, paramtype, param, 2,
                          sizeof (ctr_str), ctr_str);

         char physicalChannel[strlen (ctr_str) + strlen (device->name) + 3];
         strcpy (physicalChannel, device->name);
         strcat (physicalChannel, "/");
         strcat (physicalChannel, ctr_str);

         if (num_params >= 4)
         {
            int idle = param_to_int (context, paramtype, param, 3);
            this->idle_state = DAQmx_Val_Low;
            this->duty = 0.001;
            if (idle == 1)
            {
               this->duty = 0.999;
               this->idle_state = DAQmx_Val_High;
            }
         }

         if (this->task != 0)
         {
            DAQmxErrChk (
                  DAQmxWriteCtrFreq (this->task, //(TaskHandle taskHandle, 
                                     1,  //int32 numSampsPerChan, 
                                     0,  //bool32 autoStart, 
                                     1.0, //float64 timeout,
                                     DAQmx_Val_GroupByChannel, // dataLayout, 
                                     &(this->frequency), // float64 frequency[],
                                     &(this->duty), //  float64 dutyCycle[], 
                                     &written, // int32 *numSampsPerChanWritten,
                                     NULL)); // bool32 *reserved);
         }
         if (num_params < 2)
            break;

         bool32 taskdone;
         if (this->task != 0)
         {
            DAQmxErrChk (DAQmxStopTask (this->task));   
            DAQmxErrChk (DAQmxClearTask (this->task));  
            this->task = 0;
         }
         DAQmxErrChk (DAQmxCreateTask ("", &this->task));

         DAQmxErrChk 
              (DAQmxCreateCOPulseChanFreq (this->task, // TaskHandle  
                                           physicalChannel, // counter[], 
                                           "",  // nameToAssignToChannel[], 
                                           DAQmx_Val_Hz, //int32 units, 
                                           this->idle_state, //int32 idleState,
                                           0.0,  //float64 initialDelay, 
                                           this->frequency, //float64 freq, 
                                           this->duty)); //float64 dutyCycle);

         DAQmxErrChk (DAQmxCfgImplicitTiming
                      (this->task, DAQmx_Val_ContSamps, 1000));
         DAQmxErrChk (DAQmxStartTask (this->task));
      }
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->duty = param_to_float (context, paramtype, param, 0);
      if (this->duty > 0.999)
         this->duty = 0.999;
      if (this->duty < 0.001)
         this->duty = 0.001;

      DAQmxErrChk (DAQmxWriteCtrFreq (this->task, //(TaskHandle taskHandle, 
                                      1,        //int32 numSampsPerChan, 
                                      0,        //bool32 autoStart, 
                                      1.0,      //float64 timeout,
                                      DAQmx_Val_GroupByChannel, // dataLayout, 
                                      &(this->frequency), // float64 frequency[],
                                      &(this->duty),    //  float64 dutyCycle[], 
                                      &written, // *numSampsPerChanWritten,
                                      NULL));   // bool32 *reserved);

      write_f (context, linked_channels (context, id), this->duty);
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_float (context, paramtype, returnv, this->duty);
      break;
   }
}

/////////////////////////////////////////////////////////////////////
// counter input
/////////////////////////////////////////////////////////////////////
struct nicounter_data
{
   TaskHandle task;
   uInt32 counts;
   uInt32 zero;
};

void nicounter_func (struct nicounter_data *this,
                     const struct context_rmcios *context, int id,
                     enum function_rmcios function,
                     enum type_rmcios paramtype,
                     union param_rmcios returnv, int num_params,
                     const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for nicounter.\r\n"
                     "create nicounter ch_name\r\n"
                     "setup ch_name device_channel counter | terminal\r\n"
                     "read counter\r\n "
                     "write counter\r\n "
                     "  #read and reset\r\n"
                     "  #sends value before reset to linked channels\r\n");
      break;

   case create_rmcios: 
      if (num_params < 1)
         break;
      
      // Allocate new data:
      this = (struct nicounter_data *) malloc (sizeof (struct nicounter_data));

      // Set default values:
      this->task = 0;
      this->counts = 0;
      this->zero = 0;

      // Create the channel
      create_channel_param (context, paramtype, param, 0,
                            (class_rmcios) nicounter_func, this);
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;

      this->counts = 0;
      this->zero = 0;

      // Get the NI device for given channel:
      struct ni_device_data *device =
         get_ni_device_for_channel (param_to_int
                                    (context, paramtype, param, 0));
      if (device == NULL)
      {
         printf ("No NI device channel: %s\r\n",
                 param_to_string (context, paramtype, param, 1, 0, NULL));
         break;
      }
      else
      {
         // Build physical channel string:
         char ctr_str[30];
         param_to_string (context, paramtype, param, 1,
                          sizeof (ctr_str), ctr_str);

         char physicalChannel[strlen (ctr_str) + strlen (device->name) + 3];
         strcpy (physicalChannel, device->name);
         strcat (physicalChannel, "/");
         strcat (physicalChannel, ctr_str);

         if (this->task != 0)
         {
            DAQmxErrChk (DAQmxStopTask (this->task));  
            DAQmxErrChk (DAQmxClearTask (this->task));  
         }

         DAQmxErrChk (DAQmxCreateTask ("", &this->task));

         DAQmxErrChk (
            DAQmxCreateCICountEdgesChan (this->task,  //(TaskHandle 
                                         physicalChannel, // counter[], 
                                         "",  // nameToAssignToChannel[], 
                                         DAQmx_Val_Falling,   //int32 edge, 
                                         0,   //uInt32 initialCount, 
                                         DAQmx_Val_CountUp)); //countDirection)

         if (num_params >= 3)
         {
            DAQmxErrChk (
               DAQmxSetCICountEdgesTerm (this->task,  
                                         physicalChannel,
                                         param_to_string (context, paramtype,
                                                          param, 2, 0, NULL)));
         }
         DAQmxErrChk (DAQmxStartTask (this->task));
      }
      break;

   case read_rmcios:
   case write_rmcios:
      if (this == NULL)
         break;
      DAQmxErrChk (DAQmxReadCounterScalarU32 (this->task, //(TaskHandle, 
                                              2.0,      //float64 timeout, 
                                              &this->counts, //uInt32 *value, 
                                              NULL));   //bool32 *reserved);

      return_int (context, paramtype, returnv, this->counts - this->zero);
      if (function == write_rmcios)
      {
         write_f (context, linked_channels (context, id),
                  (this->counts - this->zero));
         // set counter to 0
         this->zero = this->counts;    
      }
      break;
   }
}

struct nido_data
{
   TaskHandle task;
   uInt8 value;
};

void nido_func (struct nido_data *this,
                const struct context_rmcios *context, int id,
                enum function_rmcios function,
                enum type_rmcios paramtype,
                union param_rmcios returnv,
                int num_params, const union param_rmcios param)
{
   switch (function)
   {
   case help_rmcios:
      return_string (context, paramtype, returnv,
                     "help for nido.\r\n"
                     "create nido newname\r\n"
                     "setup newname device_channel port line\r\n"
                     "write newname value\r\n"
                     "read newname\r\n"
                     "example: setup do1 NI1 port0 line1\r\n");
      break;

   case create_rmcios:
      if (num_params < 1)
         break;
      // Allocate new data:
      this = (struct nido_data *) malloc (sizeof (struct nido_data));

      // Set default values:
      this->task = 0;
      this->value = 0;

      // Create the channel
      create_channel_param (context, paramtype, param, 0,
                            (class_rmcios) nido_func, this);
      break;

   case setup_rmcios:
      if (this == NULL)
         break;
      if (num_params < 3)
         break;

      // Get the NI device for given channel:
      struct ni_device_data *device =
         get_ni_device_for_channel (param_to_int
                                    (context, paramtype, param, 0));
      if (device == NULL)
      {
         printf ("No NI device channel: %s\r\n",
                 param_to_string (context, paramtype, param, 1, 0, NULL));
         break;
      }
      else
      {
         // Build physical channel string:
         char port_str[30];
         char line_str[30];
         param_to_string (context, paramtype, param, 1,
                          sizeof (port_str), port_str);
         param_to_string (context, paramtype, param, 2,
                          sizeof (line_str), line_str);

         char physicalChannel[strlen (device->name) +
                              strlen (port_str) + strlen (line_str) + 4];
         strcpy (physicalChannel, device->name);
         strcat (physicalChannel, "/");
         strcat (physicalChannel, port_str);
         strcat (physicalChannel, "/");
         strcat (physicalChannel, line_str);

         if (this->task != 0)
         {  
            DAQmxErrChk (DAQmxStopTask (this->task));   
            DAQmxErrChk (DAQmxClearTask (this->task));  
         }

         DAQmxErrChk (DAQmxCreateTask ("", &this->task));
         DAQmxErrChk (DAQmxCreateDOChan (this->task,
                                         physicalChannel,
                                         "", DAQmx_Val_ChanPerLine));
         DAQmxErrChk (DAQmxStartTask (this->task));

      }
      break;

   case write_rmcios:
      if (this == NULL)
         break;
      if (num_params < 1)
         break;
      this->value = param_to_int (context, paramtype, param, 0);
      DAQmxErrChk (DAQmxWriteDigitalLines
                   (this->task, 1, 1, 10.0, DAQmx_Val_GroupByChannel,
                    &this->value, NULL, NULL));
      break;

   case read_rmcios:
      if (this == NULL)
         break;
      return_int (context, paramtype, returnv, this->value);
      break;
   }
}

void init_nidaq_channels (const struct context_rmcios *context)
{
   printf ("NIDAQ module\r\n[" VERSION_STR "] \r\n");

   create_channel_str (context, "nidev", (class_rmcios) ni_device_func, NULL); 
   create_channel_str (context, "niai", (class_rmcios) nidaq_ai_func, NULL); 
   create_channel_str (context, "niao", (class_rmcios) nidaq_ao_func, NULL);  
   create_channel_str (context, "nido", (class_rmcios) nido_func, NULL);
   create_channel_str (context, "nipwm", (class_rmcios) nipwm_func, NULL);
   create_channel_str (context, "nicounter", (class_rmcios)nicounter_func,NULL); 
}

#ifdef INDEPENDENT_CHANNEL_MODULE
// function for dynamically loading the module
void API_ENTRY_FUNC init_channels (const struct context_rmcios *context)
{
   init_nidaq_channels (context);
}
#endif
