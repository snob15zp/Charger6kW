// This is called at the PWM frequency which is 512us as of Rev 133
void CTRL_tick()
{
	// Set if the PWM needs to be stopped this tick
	int disablePWM = 0;
//RDD 3 == 2 will return 0 since three is not equal to two. The expression 5 == 5 evaluates to true and returns 1
	if (IO_getGroundFault())    // RDD !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! set with delay disablePWM according to  GroundFault: J10 on PWRboard
	{
		ctrl.pwmGroundFaultShutdown = 1;
		ctrl.groundFaultTickCount = 0;
		ctrl.pwmDisabledByOnOff = 1;
		ctrl.pwmChangeState = -1;	// Ignore changes from LCD
		if (persistentStorage.autoOn) //persistent  = always  .
		{
			persistentStorage.autoOn = 0;  //RDD autoOn - only field
			update_persistent = 1;    //RDD write to flash
		}
		disablePWM = 1;
	}
	else if (ctrl.pwmGroundFaultShutdown)
	{
		// Ground fault pin needs to be up for about 50ms before ground fault is done
		if (ctrl.groundFaultTickCount > 100)
		{
			ctrl.pwmGroundFaultShutdown = 0; //RDD Affects CTRL_isGroundFault()
		}
		else
		{
			ctrl.groundFaultTickCount++;
		}
	}

	if (SAFETY_getStatus() & 0x01)  //RDD If the fan speed was insufficient
	{
		// Negative PV current
		disablePWM = 1;

		// If PWM was on before negative current it will switch on again when current goes positive.
		// If PWM is turned on by user while the current is negative, the PWM will automatically switch on when current goes positive.
		// This might display the wrong state if the user tries to turn the output on or off while the current is negative.
		// This matches the operation of G2.
	}

	// Start-up PWM Delay is in place to ensure that the device has correctly
	// run the safety checks before it does anything else. 
#ifdef ENABLE_START_UP_PWM_DELAY        //RDD ones at PWR up
	// Added delay at start-up
	if(m_unStartUpCount<PWM_START_UP_DELAY_TICKS) {    //RDD  4C4B
		m_unStartUpCount++;
		return;
	}
#endif

	if (CFG_toggleSwitchMode)
	{
		// Toggle mode - this is not normally used at the moment.
		if (disablePWM)
		{
			// Force a small delay before pwm can be toggled
			ctrl.onOffDelayTickCount = 0;
			ctrl.onOffTickCount = 0xFFFF;
		}                                                    //RDD if disablePWM then nothing else is done in if (disablePWM)
		else                                                 //RDD if not disablePWM
		{
			int pwmWasDisabled = ctrl.pwmDisabledByOnOff; //RDD define in CTRL_init() by persistentStorage.autoOn; pwmDisabledByOnOff=!persistentStorage.autoOn
			int pwmChangeState = ctrl.pwmChangeState;

			if (IO_getOnOff())  // RDD  the button ?
			{
				if (ctrl.onOffTickCount < 0xFFFF)   // RDD calc to OxFFFF
				{
					ctrl.onOffTickCount++;
				}
			}
			else
			{
				ctrl.onOffTickCount = 0;
			}

			if (ctrl.onOffDelayTickCount < CTRL_ONOFF_DELAY_TICKS)
			{
				ctrl.onOffDelayTickCount++;
				// Must release the button before it will work if pressed before the delay ends
				ctrl.onOffTickCount = 0xFFFF;
			}
			else //RDD 	if (ctrl.onOffDelayTickCount < CTRL_ONOFF_DELAY_TICKS)
			{
				if (pwmChangeState != -1)
				{
					ctrl.pwmDisabledByOnOff = !pwmChangeState;
					persistentStorage.autoOn = (uint16_t)((ctrl.pwmDisabledByOnOff == 1) ? 0 : 1);  //Invert Logic
					update_persistent = 1;
				}
				else if (ctrl.onOffTickCount == CTRL_ONOFF_HOLD_TICKS)   //RDD delay for ctrl.pwmDisabledByOnOff
				{
					ctrl.pwmDisabledByOnOff = !ctrl.pwmDisabledByOnOff;
					persistentStorage.autoOn = (uint16_t)((ctrl.pwmDisabledByOnOff == 1) ? 0 : 1);  //Invert Logic
					update_persistent = 1;
				}

				if (ctrl.pwmDisabledByOnOff != pwmWasDisabled || pwmChangeState != -1)
				{
					ctrl.onOffDelayTickCount = 0;
					ctrl.onOffTickCount = 0xFFFF;
					ctrl.pwmChangeState = -1;
				}
			} //RDD 	if (ctrl.onOffDelayTickCount < CTRL_ONOFF_DELAY_TICKS)

			if (ctrl.pwmDisabledByOnOff)  //off PWM if ctrl.pwmDisabledByOnOff
			{
				disablePWM = 1;
			}
		}
	}
	else                 //RDD 	if (CFG_toggleSwitchMode)
	{
		// Remote shutdown or switch mode
		ctrl.pwmRemoteShutdown = IO_getOnOff(); // RDD pwmRemoteShutdown not in section before

		// pwmDisabledByOnOff is always true and autoOn is always false when ground fault shutdown
		// pwmDisabledByOnOff and autoOn couldn't be changed with a negative PV current for G2 so keep the same
		if (!disablePWM)
		{
			if (ctrl.pwmChangeState != -1)  //RDD Change PWM State on ctrl.pwmChangeState 
			{
				persistentStorage.autoOn = ctrl.pwmChangeState;
				ctrl.pwmChangeState = -1;
				update_persistent = 1;
			}

			if (ctrl.pwmRemoteShutdown)
			{
				ctrl.pwmDisabledByOnOff = 1;  //RDD on IO_getOnOff()
			}
			else
			{
				ctrl.pwmDisabledByOnOff = !persistentStorage.autoOn;
			}

			if (ctrl.pwmDisabledByOnOff)
			{
				disablePWM = 1;
			}
		}
	}

	if (disablePWM)
	{
		// PWM needs to be shutdown
		ctrl.pwmShutdown = 1;
		
		if (!IO_pwmEnabled)
		{
			// pvOcVolt is pvVolt when PWM is disabled
			meas.pvOcVolt.val = meas.pvVolt.val;
		}
		IO_disablePwmCtrl();
		MEAS_setDoUpdate(1);
		CTRL_enableTurbineLoad = 1;
		return;
	}
	else
	{
		if (ctrl.pwmShutdown)
		{
			// PWM starting after it has been stopped
			ctrl.tickCount = 0;		//Do a sample. Should give a 100ms debounce period before PWM kicks back in
		}
		ctrl.pwmShutdown = 0;
	}

	if( meas.caseTempr.val > IQ_cnst(CNTRL_OUTCURR_OVERTEMP_LIMIT_1) && meas.caseTempr.val < IQ_cnst(CNTRL_OUTCURR_OVERTEMP_LIMIT_2) ) // De-rate output current maximum by 20% if inductor is above 90C but below 100C.
	{
		unCntrlOutCurrLimit=IQ_cnst( (CNTRL_OUTCURR_LIMIT*CNTRL_OUTCURR_OVERTEMP_REDUC_1) / MEAS_OUTCURR_BASE ); 
		unCntrlOutCurrSwOffPoint=IQ_cnst( ((CNTRL_OUTCURR_LIMIT*CNTRL_OUTCURR_OVERTEMP_REDUC_1)*CNTRL_OUTCURR_HYST_PERCENT/100.) / MEAS_OUTCURR_BASE ); 
	}
	else if( meas.caseTempr.val > IQ_cnst(CNTRL_OUTCURR_OVERTEMP_LIMIT_2)) // De-rate output current maximum by 30% if inductor is above 100C (Additional Temperature Failsafe before 120C Shutdown).
	{
		unCntrlOutCurrLimit=IQ_cnst( (CNTRL_OUTCURR_LIMIT*CNTRL_OUTCURR_OVERTEMP_REDUC_2) / MEAS_OUTCURR_BASE ); 
		unCntrlOutCurrSwOffPoint=IQ_cnst( ((CNTRL_OUTCURR_LIMIT*CNTRL_OUTCURR_OVERTEMP_REDUC_2)*CNTRL_OUTCURR_HYST_PERCENT/100.) / MEAS_OUTCURR_BASE ); 		
	}
	else 
	{
		unCntrlOutCurrLimit=IQ_cnst( CNTRL_OUTCURR_LIMIT / MEAS_OUTCURR_BASE ); 
		unCntrlOutCurrSwOffPoint=IQ_cnst( (CNTRL_OUTCURR_LIMIT*CNTRL_OUTCURR_HYST_PERCENT/100.) / MEAS_OUTCURR_BASE ); 
	}
	
   // Section to check the current limitation
#ifdef APPLY_CURRENT_LIMITATION   
   if( meas.outCurr.val > unCntrlOutCurrLimit ) {  
   
	  // Set the limitation flag
	  bCurrentLimiting=true;
   
	  MEAS_setDoUpdate(1);
   
      unCurrLimitHysterisis++;
      
      // Only apply after the hysterisis count has expired.
      // This is to account for the system delays
      if(unCurrLimitHysterisis>CURR_LIMIT_HYST_COUNT) {
      
         // Increment the MPPT voltage by 200mV
 	   	if(CTRL_MpptSamplePtNow>0) {
			CTRL_MpptSamplePtNow += IQ_cnst( 0.01*(meas.outCurr.val-unCntrlOutCurrLimit)/ MEAS_PVVOLT_BASE );//IQ_cnst( 0.1 / MEAS_PVVOLT_BASE );
			PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );
		}
		else {
//			ctrl.pwmShutdown = 1;
		}
            
         unCurrLimitHysterisis=0;
      }
   }
   else if(bCurrentLimiting==true) {
	   
      unCurrLimitHysterisis++;
	  
	  MEAS_setDoUpdate(1);
      
      // Only apply after the hysterisis count has expired.
      // This is to account for the system delays
      if(unCurrLimitHysterisis>CURR_LIMIT_HYST_COUNT) {
		 CTRL_MpptSamplePtNow -= IQ_cnst( 0.1 / MEAS_PVVOLT_BASE );
		 PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );
		 unCurrLimitHysterisis=0;
	  }
	   
	   // Check if we need to switch off the current limitation
       if(meas.outCurr.val < unCntrlOutCurrSwOffPoint) {
		   bCurrentLimiting=false;
		   ctrl.tickCount = CTRL_SAMPLE_PERIOD_TICKS-1; 
	   }
   }

   
#endif  
   
#ifndef DBG_HARDCODED_VIN_SETPOINT
#ifdef APPLY_CURRENT_LIMITATION      
  	else if ( ctrl.tickCount == 0 )  // 2018-10-17 Added Current Limiter.
#else      
	if ( ctrl.tickCount == 0 )
#endif      
	{
		// Go open-circuit
		IO_disablePwmCtrl();
		// Don't update measurements for a while
		MEAS_setDoUpdate( 0 );
	}
	else if ( ctrl.tickCount > 0 && ctrl.tickCount < CTRL_OPEN_CIRCUIT_TIME_TICKS )
	{
		// Measuring Voc
		meas.pvOcVolt.val = MEAS_filterFast( meas.pvOcVolt.val, meas.pvVolt.valPreFilter );
	}
	else if ( ctrl.tickCount == CTRL_OPEN_CIRCUIT_TIME_TICKS )
	{
		// Set MpptSamplePt and VinLim based on measured Voc
#ifdef DBG_FULL_ON
		PWM_setMpptSamplePt( IQ_cnst(0.1) );
		PWM_setVinLim( IQ_cnst(0.5) );
#else
		// 1.04 added a ramp on the mppt voltage setpoint after a setpoint
		// to try to limit the overshoot that occurs due to integrator windup in the analog control loop
		//PWM_setMpptSamplePt( IQ_mpy( meas.pvOcVolt.val, ctrl.pvVoltFrac ) );
		CTRL_MpptSamplePtTarget = IQ_mpy( meas.pvOcVolt.val, ctrl.pvVoltFrac );
		CTRL_MpptSamplePtNow = meas.pvOcVolt.val;
		PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );

		//PWM_setVinLim( ctrl.pvVinLim );
#endif
		//PWM_setFlTrim( IQ_cnst(0.5) );
		//PWM_setFlTrim( IQ_cnst(0.0) );

		// Enable output
		IO_enablePwmCtrl();
	}
	else if ( ctrl.tickCount > CTRL_OPEN_CIRCUIT_TIME_TICKS && ctrl.tickCount < CTRL_NO_MEAS_TIME_TICKS)
	{
		//1.04 ramping the MPPT setpoint down by 200mV/ms
		if(CTRL_MpptSamplePtNow > CTRL_MpptSamplePtTarget)
		{
			CTRL_MpptSamplePtNow -= IQ_cnst( 0.1 / MEAS_PVVOLT_BASE );
			PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );
		}
	}
	else if ( ctrl.tickCount == CTRL_NO_MEAS_TIME_TICKS )
	{
		// in case the mppt sample point didn't reach the target
		PWM_setMpptSamplePt( CTRL_MpptSamplePtTarget );
		// Go back to updating measurements as normal
		MEAS_setDoUpdate( 1 );
	}
	else if ( ctrl.tickCount > CTRL_NO_MEAS_TIME_TICKS )
	{
		PWM_setVinLim( IQ_mpy( IQ_cnst( 1.1 ), meas.pvOcVolt.val ));
	}
#else
#ifdef APPLY_CURRENT_LIMITATION         
	else if (firstMPPTPoint < 1)  // DM : 2018-10-17 Added Current Limiter.
#else      
	if (firstMPPTPoint < 1)
#endif      
	{
		if ( ctrl.tickCount == 0 )
		{
			// Go open-circuit
			IO_disablePwmCtrl();
			// Don't update measurements for a while
			MEAS_setDoUpdate( 0 );
		}
		else if ( ctrl.tickCount > 0 && ctrl.tickCount < CTRL_OPEN_CIRCUIT_TIME_TICKS )
		{
			// Measuring Voc
			meas.pvOcVolt.val = MEAS_filterFast( meas.pvOcVolt.val, meas.pvVolt.valPreFilter );
		}
		else if ( ctrl.tickCount == CTRL_OPEN_CIRCUIT_TIME_TICKS )
		{
			// Set MpptSamplePt and VinLim based on measured Voc
			// 1.04 added a ramp on the mppt voltage setpoint after a setpoint
			// to try to limit the overshoot that occurs due to integrator windup in the analog control loop
			//PWM_setMpptSamplePt( IQ_mpy( meas.pvOcVolt.val, ctrl.pvVoltFrac ) );
			CTRL_MpptSamplePtTarget = fixedMpVolt;
			CTRL_MpptSamplePtNow = meas.pvOcVolt.val;
			if (CTRL_MpptSamplePtNow < fixedMpVolt)
			{
				CTRL_MpptSamplePtNow = fixedMpVolt;
			}
			PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );

			// Enable output
			IO_enablePwmCtrl();
			CTRL_enableTurbineLoad = 0;
		}
		else if ( ctrl.tickCount > CTRL_OPEN_CIRCUIT_TIME_TICKS && ctrl.tickCount < CTRL_NO_MEAS_TIME_TICKS)
		{
			//1.04 ramping the MPPT setpoint down by 200mV/ms
			if(CTRL_MpptSamplePtNow > CTRL_MpptSamplePtTarget)
			{
				CTRL_MpptSamplePtNow -= IQ_cnst( 0.1 / MEAS_PVVOLT_BASE );
				PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );
			}
			//else if(CTRL_MpptSamplePtNow < CTRL_MpptSamplePtTarget)
			//{
			//	CTRL_MpptSamplePtNow += IQ_cnst( 0.1 / MEAS_PVVOLT_BASE );
			//	PWM_setMpptSamplePt( CTRL_MpptSamplePtNow );
			//}
		}
		else if ( ctrl.tickCount == CTRL_NO_MEAS_TIME_TICKS )
		{
			// in case the mppt sample point didn't reach the target
			PWM_setMpptSamplePt( CTRL_MpptSamplePtTarget );
			// Go back to updating measurements as normal
			MEAS_setDoUpdate( 1 );
		}
		else if ( ctrl.tickCount > CTRL_NO_MEAS_TIME_TICKS )
		{
			//firstMPPTPoint=1;
			PWM_setVinLim(IQ_mpy( IQ_cnst( 1.1 ),fixedOcVolt));
		}
	}
		
	/* 
	meas.pvOcVolt.val = ctrl.pvVoltFrac;
	MEAS_setDoUpdate( 1 );
	PWM_setMpptSamplePt(ctrl.pvVoltFrac);
	IO_enablePwmCtrl();
	*/
#endif

	
	//{
		//1.04 (19/04/2012) removed the vinlim stuff as it is now redundant and sometimes causes strange behaviour.
		
		//PWM_setVinLim( IQ_mpy( IQ_cnst( 1.1 ), meas.pvOcVolt.val ));
				
		/*
		// Run VinLim control loop based on PV current
		pvVinLimMax = IQ_mpy( IQ_cnst( 1.1 ), meas.pvOcVolt.val );
		//pvVinLimMax = meas.pvOcVolt.val;
		pvVinLimMin = IQ_mpy( IQ_cnst( MEAS_OUTVOLT_TO_PVVOLT ), meas.outVolt.val );
		//pvVinLimMin = IQ_cnst( 60.0 / MEAS_PVVOLT_BASE );
		if ( pvVinLimMax <= pvVinLimMin ) pvVinLimMax = pvVinLimMin + 1;

		// Run integrator if direction of increment is correct
		//if (	( meas.pvCurr.val < 0 && ctrl.pvVinLimSat >= 0 ) 
		//	 ||	( meas.pvCurr.val > 0 && ctrl.pvVinLimSat <= 0 ) )
		{
			pvVinIntIncr = IQ_mpy( IQ_cnst(CTRL_KI_VINLIM), meas.pvCurr.val );
			if ( pvVinIntIncr == 0 ) pvVinIntIncr = IQ_sign( meas.pvCurr.val );
			ctrl.pvVinLimInt += pvVinIntIncr;
		}

		// Saturate integrator intependendly
		if ( ctrl.pvVinLimInt > pvVinLimMax ) ctrl.pvVinLimInt = pvVinLimMax;
		if ( ctrl.pvVinLimInt < pvVinLimMin ) ctrl.pvVinLimInt = pvVinLimMin;

		ctrl.pvVinLim = IQ_mpy( IQ_cnst(CTRL_KP_VINLIM), meas.pvCurr.val ) + ctrl.pvVinLimInt;

		ctrl.pvVinLimSat = 0;
		if ( ctrl.pvVinLim > pvVinLimMax ) { ctrl.pvVinLim = pvVinLimMax; ctrl.pvVinLimSat = 1; }
		if ( ctrl.pvVinLim < pvVinLimMin ) { ctrl.pvVinLim = pvVinLimMin; ctrl.pvVinLimSat = -1; }
		
		
		PWM_setVinLim( ctrl.pvVinLim );
		//PWM_setVinLim( pvVinLimMax );
		//PWM_setVinLim( IQ_cnst( 82.0 / MEAS_PVVOLT_BASE ) );
		*/
	//}

#ifdef DEBUG_LED_SAMPLE
	if ( ctrl.tickCount > CTRL_ABOUT_TO_SAMPLE_TICKS )
	{
		IO_setRelay(1);
	}
	else 
	{
		IO_setRelay(0);
	}
#endif
	
	ctrl.tickCount++;

	if ( ctrl.tickCount == CTRL_SAMPLE_PERIOD_TICKS )
	{
		ctrl.tickCount = 0;
	}


}

