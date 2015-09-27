//#define __SATAN_USES_FXP
#define __SATAN_USES_FLOATS

#include "libtestbench_timer.c"
#include "comprezza.c"
#include "libtestbench.c"

#define CHANNELS 2
#define STEP_LENGTH 240
#define MAX_STEPS  1837 * 4
#define TEST_LENGTH (STEP_LENGTH * MAX_STEPS)
#define BENCH_FREQUENCY 44100

FTYPE input_signal[CHANNELS * TEST_LENGTH];
FTYPE output_signal[CHANNELS * TEST_LENGTH];
int16_t riffdata[TEST_LENGTH * 2];

int main(int argc, char **argv) {
	printf("hello\n");
	struct mockery m;

	printf("Preparing mockery...\n");
	if(prepare_mockery(&m, STEP_LENGTH, "comprezza")) {
		printf("Failed to initiate mockery of module comprezza.\n");
		exit(1);
	}

	printf("Creating mock signals...\n");
	if(create_mock_input_signal(
		   &m, "Stereo",
		   _0D,
		   CHANNELS,
		   FTYPE_RESOLUTION,
		   BENCH_FREQUENCY,
		   TEST_LENGTH,
		   input_signal
		   )) {
		printf("Failed to create mock input signal.\n");
		exit(2);
	}
	if(create_mock_output_signal(
		   &m, "Stereo",
		   _0D,
		   CHANNELS,
		   FTYPE_RESOLUTION,
		   BENCH_FREQUENCY,
		   TEST_LENGTH,
		   output_signal
		   )) {
		printf("Failed to create mock output signal.\n");
		exit(3);
	}

	memset(input_signal, 0, sizeof(input_signal));
	memset(output_signal, 0, sizeof(output_signal));

	printf("Will load wav file...\n");

	{
		int rd;

		RIFF_WAVE_FILE_t rwf;

		RIFF_open_file(&rwf, "modulator44.wav");

		rd = RIFF_read_data(&rwf, riffdata, TEST_LENGTH);

		printf("WAV channels: %d, samples: %d  -> channels: %d\n",
		       rwf.channels, rwf.samples, CHANNELS);

		convert_pcm_to_mock_signal(
			input_signal, CHANNELS, TEST_LENGTH,

			riffdata,
			rwf.channels, rwf.samples);

		RIFF_close_file(&rwf);
		printf("   read: %d\n", rd);
		printf("data[0] = %d\n", riffdata[0]);
		printf("data[1] = %d\n", riffdata[1]);
		printf("data[2] = %d\n", riffdata[2]);
		printf("data[3] = %d\n", riffdata[3]);
	}


	make_a_mockery(&m, MAX_STEPS);

	write_stereo_wav(input_signal, TEST_LENGTH, "/tmp/mocking_in.wav");
	write_stereo_wav(output_signal, TEST_LENGTH, "/tmp/mocking_out.wav");

	printf("\n\n");
	printf("input: %p\n", input_signal);
	printf("output: %p\n", output_signal);

}
