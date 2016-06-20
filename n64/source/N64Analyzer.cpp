#include "N64Analyzer.h"
#include "N64AnalyzerSettings.h"
#include <AnalyzerChannelData.h>
#include <math.h>
#include <cmath>

N64Analyzer::N64Analyzer()
:	Analyzer(),  
	mSettings( new N64AnalyzerSettings() ),
	mSimulationInitilized( false )
{
	SetAnalyzerSettings( mSettings.get() );
}

N64Analyzer::~N64Analyzer()
{
	KillThread();
}

double round( double value )
{
return floor( value + 0.5 );
}

//void N64Analyzer::WorkerThread()
//{
//	mResults.reset( new N64AnalyzerResults( this, mSettings.get() ) );
//	SetAnalyzerResults( mResults.get() );
//	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );
//
//	mSampleRateHz = GetSampleRate();
//	double samples_per_bit = mSampleRateHz / bitrate;
//	U32 samples_to_first_center_of_first_data_bit = U32( 1.5 * double( mSampleRateHz ) / double( bitrate ) );
//	mData = GetAnalyzerChannelData( mSettings->mInputChannel );
//
//	if( mData->GetBitState() == BIT_LOW )
//		mData->AdvanceToNextEdge();
//
//	int state = CONTROLLER_STOP_BIT;
//	bool idle = true;
//	U32 data = 0;
//	for (;;) {
//		U64 start, middle, end, first, second;
//		int bit = NO_BIT;
//
//		mData->AdvanceToNextEdge();
//		start = mData->GetSampleNumber();
//		middle = 0;
//
//		// idle + console stop doesn't give us the first half of the signal
//		// so we step back to align everything
//		if (idle && state == CONSOLE_STOP_BIT) {
//			end = mData->GetSampleOfNextEdge();
//			first = round((double)(end - start) / samples_per_bit);
//			if (first == 1 || first == 3) {
//				middle = start;
//				idle = false;
//
//				if (first == 1) {
//					start -= samples_per_bit * 3;
//				} else if (first == 3) {
//					start -= samples_per_bit * 1;
//				}
//			}
//		}
//
//		if (!middle) {
//			mData->AdvanceToNextEdge();
//			middle = mData->GetSampleNumber();	
//		}
//		first = round((double)(middle - start) / samples_per_bit);
//		end = std::min(mData->GetSampleOfNextEdge(), (U64)(middle + (4 - first) * samples_per_bit));
//		second = round((double)(end - middle) / samples_per_bit);
//
//		if (bit == NO_BIT) {
//			if (first == 1 && second == 3) {
//				bit = ONE_BIT;
//				data = data << 1 | 1;
//			} else if (first == 3 && second == 1) {
//				bit = ZERO_BIT;
//				data = data << 1;
//			} else if (first == 1 && second == 2) {
//				idle = true;
//				state = bit = CONSOLE_STOP_BIT;
//				// consume an edge because we're on the wrong side of the cycle
//				mData->AdvanceToNextEdge();
//
//			} else if (first == 2 && (second == 1 || second == 2)) {
//				if (second == 2)
//					end -= samples_per_bit;
//
//				idle = true;
//				state = bit = CONTROLLER_STOP_BIT;
//			}
//		}
//
//		if (bit == NO_BIT)
//			continue;
//
//		// add a marker
//		U64 halfway = start + (end - start) / 2.0;
//		mResults->AddMarker( halfway, AnalyzerResults::Dot, mSettings->mInputChannel );
//
//		Frame frame;
//		frame.mData1 = bit;
//		frame.mFlags = 0;
//		frame.mStartingSampleInclusive = start;
//		frame.mEndingSampleInclusive = end;
//
//		mResults->AddFrame( frame );
//		mResults->CommitResults();
//		ReportProgress( frame.mEndingSampleInclusive );
//	}
//	return;
//
//	for( ; ; )
//	{
//		U8 data = 0;
//		U8 mask = 1 << 7;
//		
//		mData->AdvanceToNextEdge(); //falling edge -- beginning of the start bit
//
//		U64 starting_sample = mData->GetSampleNumber();
//
//		mData->Advance( samples_to_first_center_of_first_data_bit );
//
//		for( U32 i=0; i<8; i++ )
//		{
//			//let's put a dot exactly where we sample this bit:
//			mResults->AddMarker( mData->GetSampleNumber(), AnalyzerResults::Dot, mSettings->mInputChannel );
//
//			if( mData->GetBitState() == BIT_HIGH )
//				data |= mask;
//
//			mData->Advance( samples_per_bit );
//
//			mask = mask >> 1;
//		}
//
//
//		//we have a byte to save. 
//		Frame frame;
//		frame.mData1 = data;
//		frame.mFlags = 0;
//		frame.mStartingSampleInclusive = starting_sample;
//		frame.mEndingSampleInclusive = mData->GetSampleNumber();
//
//		mResults->AddFrame( frame );
//		mResults->CommitResults();
//		ReportProgress( frame.mEndingSampleInclusive );
//	}
//}

void N64Analyzer::WorkerThread()
{
	mResults.reset( new N64AnalyzerResults( this, mSettings.get() ) );
	SetAnalyzerResults( mResults.get() );
	mResults->AddChannelBubblesWillAppearOn( mSettings->mInputChannel );

	mSampleRateHz = GetSampleRate();
	mData = GetAnalyzerChannelData( mSettings->mInputChannel );
	U64 one_us_low = (U64)((0.75 * (double)mSampleRateHz) / 1000000.0);
	U64 one_us_exact = (U64)((1.0 * (double)mSampleRateHz) / 1000000.0);
	U64 one_us_high = (U64)((1.25 * (double)mSampleRateHz) / 1000000.0);
	U64 two_us_low = (U64)((1.75 * (double)mSampleRateHz) / 1000000.0);
	U64 two_us_high = (U64)((2.25 * (double)mSampleRateHz) / 1000000.0);
	U64 three_us_low = (U64)((2.75 * (double)mSampleRateHz) / 1000000.0);
	U64 three_us_exact = (U64)((3.0 * (double)mSampleRateHz) / 1000000.0);
	U64 three_us_high = (U64)((3.25 * (double)mSampleRateHz) / 1000000.0);


	// Move forward until the line is high
	if( mData->GetBitState() == BIT_LOW )
		mData->AdvanceToNextEdge();

	// Move until it is low
	mData->AdvanceToNextEdge();

	// We're now on a falling edge
	for (;;) {
		U64 low_start, low_end, high_end;

		low_start = mData->GetSampleNumber();

		// Measure how long it takes to go high
		mData->AdvanceToNextEdge();
		low_end = mData->GetSampleNumber();

		U64 low_duration = low_end - low_start;

		// Measure how long it is high for
		mData->AdvanceToNextEdge();
		high_end = mData->GetSampleNumber();
		U64 high_duration = high_end - low_end;

		if (low_duration >= one_us_low && low_duration <= one_us_high)
		{
			if (high_duration >= three_us_low)
			{
				// Decide where to call the end of this bit
				U64 bit_end = high_end;

				if (high_duration > three_us_high)
				{
					bit_end = low_end + three_us_exact;
				}

				U64 halfway = low_start + (bit_end - low_start) / 2.0;
				mResults->AddMarker( halfway, AnalyzerResults::Dot, mSettings->mInputChannel );

				Frame frame;
				frame.mData1 = 1;
				frame.mFlags = 0;
				frame.mStartingSampleInclusive = low_start;
				frame.mEndingSampleInclusive = bit_end;

				mResults->AddFrame( frame );
				mResults->CommitResults();
			}
			else 
			{
				// Might be a consolestop bit? 
				// Put a '3' on just the low part
				Frame frame;
				frame.mData1 = 3;
				frame.mFlags = 0;
				frame.mStartingSampleInclusive = low_start;
				frame.mEndingSampleInclusive = low_end;

				mResults->AddFrame( frame );
				mResults->CommitResults();
			}
		}
		else if (low_duration >= two_us_low && low_duration <= two_us_high)
		{
			// Controller stop bit
			// Mark only the low portion
			Frame frame;
			frame.mData1 = 2;
			frame.mFlags = 0;
			frame.mStartingSampleInclusive = low_start;
			frame.mEndingSampleInclusive = low_end;

			mResults->AddFrame( frame );
			mResults->CommitResults();
		}
		else if (low_duration >= three_us_low && low_duration <= three_us_high)
		{
			if (high_duration >= one_us_low)
			{
				// Decide where to call the end of this bit
				U64 bit_end = high_end;

				if (high_duration > one_us_high)
				{
					bit_end = low_end + one_us_exact;
				}

				U64 halfway = low_start + (bit_end - low_start) / 2.0;
				mResults->AddMarker( halfway, AnalyzerResults::Dot, mSettings->mInputChannel );

				Frame frame;
				frame.mData1 = 0;
				frame.mFlags = 0;
				frame.mStartingSampleInclusive = low_start;
				frame.mEndingSampleInclusive = bit_end;

				mResults->AddFrame( frame );
				mResults->CommitResults();
			}
			else 
			{
				// High portion was too short? should we ignore this?
			}
		}


		//bool success_flag = false;
		//U64 bit_val = 0;
		//if (duration >= one_us_low && duration <= one_us_high)
		//{
		//	success_flag = true;
		//	bit_val = 1;
		//}
		//else if (duration >= two_us_low && duration <= two_us_high)
		//{
		//	success_flag = true;
		//	bit_val = 2;
		//}
		//else if (duration >= three_us_low && duration <= three_us_high)
		//{
		//	success_flag = true;
		//	bit_val = 3;
		//}

		//// 

		//if (success_flag)
		//{
		//	
		//}

		ReportProgress( high_end );

		// We are now on a falling edge which will start the next loop correctly
	}
	return;

	double samples_per_bit = mSampleRateHz / bitrate;
	U32 samples_to_first_center_of_first_data_bit = U32( 1.5 * double( mSampleRateHz ) / double( bitrate ) );
	mData = GetAnalyzerChannelData( mSettings->mInputChannel );

	if( mData->GetBitState() == BIT_LOW )
		mData->AdvanceToNextEdge();

	int state = CONTROLLER_STOP_BIT;
	bool idle = true;
	U32 data = 0;
	for (;;) {
		U64 start, middle, end, first, second;
		int bit = NO_BIT;

		mData->AdvanceToNextEdge();
		start = mData->GetSampleNumber();
		middle = 0;

		// idle + console stop doesn't give us the first half of the signal
		// so we step back to align everything
		if (idle && state == CONSOLE_STOP_BIT) {
			end = mData->GetSampleOfNextEdge();
			first = round((double)(end - start) / samples_per_bit);
			if (first == 1 || first == 3) {
				middle = start;
				idle = false;

				if (first == 1) {
					start -= samples_per_bit * 3;
				} else if (first == 3) {
					start -= samples_per_bit * 1;
				}
			}
		}

		if (!middle) {
			mData->AdvanceToNextEdge();
			middle = mData->GetSampleNumber();	
		}
		first = round((double)(middle - start) / samples_per_bit);
		end = std::min(mData->GetSampleOfNextEdge(), (U64)(middle + (4 - first) * samples_per_bit));
		second = round((double)(end - middle) / samples_per_bit);

		if (bit == NO_BIT) {
			if (first == 1 && second == 3) {
				bit = ONE_BIT;
				data = data << 1 | 1;
			} else if (first == 3 && second == 1) {
				bit = ZERO_BIT;
				data = data << 1;
			} else if (first == 1 && second == 2) {
				idle = true;
				state = bit = CONSOLE_STOP_BIT;
				// consume an edge because we're on the wrong side of the cycle
				mData->AdvanceToNextEdge();

			} else if (first == 2 && (second == 1 || second == 2)) {
				if (second == 2)
					end -= samples_per_bit;

				idle = true;
				state = bit = CONTROLLER_STOP_BIT;
			}
		}

		if (bit == NO_BIT)
			continue;

		// add a marker
		U64 halfway = start + (end - start) / 2.0;
		mResults->AddMarker( halfway, AnalyzerResults::Dot, mSettings->mInputChannel );

		Frame frame;
		frame.mData1 = bit;
		frame.mFlags = 0;
		frame.mStartingSampleInclusive = start;
		frame.mEndingSampleInclusive = end;

		mResults->AddFrame( frame );
		mResults->CommitResults();
		ReportProgress( frame.mEndingSampleInclusive );
	}
	return;

}

bool N64Analyzer::NeedsRerun()
{
	return false;
}

U32 N64Analyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels )
{
	if( mSimulationInitilized == false )
	{
		mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 N64Analyzer::GetMinimumSampleRateHz()
{
	return bitrate * 4;
}

const char* N64Analyzer::GetAnalyzerName() const
{
	return "N64 Controller";
}

const char* GetAnalyzerName()
{
	return "N64 Controller";
}

Analyzer* CreateAnalyzer()
{
	return new N64Analyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
	delete analyzer;
}
