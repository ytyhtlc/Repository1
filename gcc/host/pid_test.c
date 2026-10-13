#include "PID.h"

#include <stdio.h>
#include <stdlib.h>

static void expect_near(const char *name, float got, float want)
{
	float delta = got - want;
	if (delta < 0) {
		delta = -delta;
	}
	if (delta > 0.001f) {
		fprintf(stderr, "%s: got %f, want %f\n", name, got, want);
		exit(1);
	}
	printf("%s=%g\n", name, got);
}

int main(void)
{
	PID_t pid = {0};

	/* Proportional term plus the output offset used by the balance controller. */
	pid.Kp = 2.0f;
	pid.Ki = 0.0f;
	pid.Kd = 0.0f;
	pid.plus = 1.0f;
	pid.OutMax = 100.0f;
	pid.OutMin = -100.0f;
	pid.Target = 10.0f;
	pid.Actual = 0.0f;
	PID_Update(&pid);
	expect_near("proportional_plus_offset", pid.Out, 21.0f);

	/* Output is clamped to OutMax. */
	pid.Kp = 10.0f;
	pid.plus = 0.0f;
	pid.OutMax = 15.0f;
	pid.OutMin = -15.0f;
	pid.Target = 100.0f;
	pid.Actual = 0.0f;
	pid.Actual1 = 0.0f;
	pid.ErrorInt = 0.0f;
	PID_Update(&pid);
	expect_near("clamped_high", pid.Out, 15.0f);

	/* Negative error subtracts the offset and respects OutMin. */
	pid.Kp = 2.0f;
	pid.plus = 1.0f;
	pid.OutMax = 100.0f;
	pid.OutMin = -100.0f;
	pid.Target = 0.0f;
	pid.Actual = 10.0f;
	pid.Actual1 = 0.0f;
	PID_Update(&pid);
	expect_near("negative_offset", pid.Out, -21.0f);

	/* Derivative acts on measured value, not error. */
	pid.Kp = 0.0f;
	pid.plus = 0.0f;
	pid.Kd = 2.0f;
	pid.Target = 0.0f;
	pid.Actual = 1.0f;
	pid.Actual1 = 0.0f;
	PID_Update(&pid);
	expect_near("derivative_on_actual", pid.Out, -2.0f);

	return 0;
}
