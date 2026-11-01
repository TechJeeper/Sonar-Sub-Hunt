#include "aim.h"

static AimState s_aim = { .heading_deg = 0, .elev_deg = 0, .compass_ok = false };
static int16_t s_accel_heading = 0;

static int16_t wrap_deg(int16_t d) {
  while (d < 0) d += 360;
  while (d >= 360) d -= 360;
  return d;
}

static int16_t elev_from_accel(int16_t ax, int16_t ay, int16_t az) {
  // Pitch from gravity: watch face roughly upright → elev near 0.
  // Tilting the crown up/down changes az vs ay.
  (void)ax;
  int32_t mag = ay * ay + az * az;
  if (mag < 100) return s_aim.elev_deg;

  // Approximate asin(az / |g|) in degrees via clamp + scale.
  // az ≈ −1000 when face-up flat; ≈ 0 when upright.
  int32_t e = (-az) / 11;  // ~±90
  if (e > 70) e = 70;
  if (e < -70) e = -70;
  return (int16_t)e;
}

static void accel_handler(AccelData *data, uint32_t num_samples) {
  if (!num_samples) return;
  AccelData *s = &data[num_samples - 1];

  s_aim.elev_deg = elev_from_accel(s->x, s->y, s->z);

#if !defined(PBL_COMPASS)
  // No magnetometer: derive relative heading from wrist yaw (x/y).
  // atan2(x, y) via trig lookup — coarse but sweeps as you turn.
  int32_t angle = atan2_lookup(s->y, s->x);
  s_accel_heading = (int16_t)(TRIGANGLE_TO_DEG(angle));
  s_aim.heading_deg = wrap_deg(s_accel_heading);
  s_aim.compass_ok = true;
#else
  (void)s_accel_heading;
#endif
}

#if defined(PBL_COMPASS)
static void compass_handler(CompassHeadingData heading) {
  if (heading.compass_status == CompassStatusDataInvalid) {
    s_aim.compass_ok = false;
    return;
  }
  // magnetic_heading is a trig angle clockwise from north.
  int16_t deg = (int16_t)TRIGANGLE_TO_DEG(heading.magnetic_heading);
  s_aim.heading_deg = wrap_deg(deg);
  s_aim.compass_ok = (heading.compass_status == CompassStatusCalibrated) ||
                     (heading.compass_status == CompassStatusCalibrating);
}
#endif

void aim_init(void) {
  accel_data_service_subscribe(1, accel_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);

#if defined(PBL_COMPASS)
  compass_service_set_heading_filter(DEG_TO_TRIGANGLE(2));
  compass_service_subscribe(compass_handler);
#endif
}

void aim_deinit(void) {
  accel_data_service_unsubscribe();
#if defined(PBL_COMPASS)
  compass_service_unsubscribe();
#endif
}

AimState aim_get(void) {
  return s_aim;
}
