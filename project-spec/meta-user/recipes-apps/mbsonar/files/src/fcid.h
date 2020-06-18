#include "datadict.h"

#define MBIW MAKE_FOURCHAR_ID('M','B','I','W') /* mbs in water - in/or out for 3 seconds (with YBIW) */
#define MBSW MAKE_FOURCHAR_ID('M','B','S','W') /* mbs in water - actual state */
#define MSWR MAKE_FOURCHAR_ID('M','S','W','R') /* software update ready from control head */
#define SBMR MAKE_FOURCHAR_ID('S','B','M','R') /* mode as reported by gyro when in to full/down/forward/outlook (with YBMR) */
#define SBRD MAKE_FOURCHAR_ID('S','B','R','D') /* down range - real value when in auto mode (with YBRD) */
#define SBRF MAKE_FOURCHAR_ID('S','B','R','F') /* forward range - real value when in auto mode (with YBRF) */
#define SEDU MAKE_FOURCHAR_ID('S','E','D','U') /* burn-in date code */
#define SEIU MAKE_FOURCHAR_ID('S','E','I','U') /* post-burn-in line number */
#define SENU MAKE_FOURCHAR_ID('S','E','N','U') /* sequential unit id */
#define SM1D MAKE_FOURCHAR_ID('S','M','1','D') /* mode 1 (full) - depth range in meters (with YM1D) */
#define SM1F MAKE_FOURCHAR_ID('S','M','1','F') /* mode 1 (full) - forward range in meters (with YM1F) */
#define SM2D MAKE_FOURCHAR_ID('S','M','2','D') /* mode 2 (down) - depth range in meters (with YM2D) */
#define SM2L MAKE_FOURCHAR_ID('S','M','2','L') /* mbs chirp frequency start (with YM2L) */
#define SM2U MAKE_FOURCHAR_ID('S','M','2','U') /* mbs chirp frequency end (with YM2U) */
#define SM3D MAKE_FOURCHAR_ID('S','M','3','D') /* mode 3 (forward) - depth range in meters (with YM3D) */
#define SM3F MAKE_FOURCHAR_ID('S','M','3','F') /* mode 3 (forward) - forward range in meters (with YM3F) */
#define SM4F MAKE_FOURCHAR_ID('S','M','4','F') /* mode 4 (outlook) - forward range in meters (with YM4F) */
#define SMBN MAKE_FOURCHAR_ID('S','M','B','N') /* Sonar enabled (0==DISABLED completely - with YMBN) */
#define SMDL MAKE_FOURCHAR_ID('S','M','D','L') /* model ID */
#define SMLM MAKE_FOURCHAR_ID('S','M','L','M') /* mode - auto/full/down/forward/outlook - with YMLM */
#define SVER MAKE_FOURCHAR_ID('S','V','E','R') /* version number */
#define YBIW MAKE_FOURCHAR_ID('Y','B','I','W') /* mbs in water - in/or out for 3 seconds (with MBIW) */
#define YBRD MAKE_FOURCHAR_ID('Y','B','R','D') /* down range - real value when in auto mode (with MBRD) */
#define YBRF MAKE_FOURCHAR_ID('Y','B','R','F') /* forward range - real value when in auto mode (with MBRF) */
#define YBMR MAKE_FOURCHAR_ID('Y','B','M','R') /* mode as reported by gyro when in to full/down/forward/outlook (with SBMR) */
#define YM1D MAKE_FOURCHAR_ID('Y','M','1','D') /* mode 1 (full) - depth range in meters (with SM1D) */
#define YM1F MAKE_FOURCHAR_ID('Y','M','1','F') /* mode 1 (full) - forward range in meters (with SM1F) */
#define YM2D MAKE_FOURCHAR_ID('Y','M','2','D') /* mode 2 (down) - depth range in meters (with SM2D) */
#define YM2L MAKE_FOURCHAR_ID('Y','M','2','L') /* mbs chirp frequency start(with SM2L) */
#define YM2U MAKE_FOURCHAR_ID('Y','M','2','U') /* mbs chirp frequency end (with SM2U) */
#define YM3D MAKE_FOURCHAR_ID('Y','M','3','D') /* mode 3 (forward) - depth range in meters (with SM3D) */
#define YM3F MAKE_FOURCHAR_ID('Y','M','3','F') /* mode 3 (forward) - forward range in meters (with SM3F) */
#define YM4F MAKE_FOURCHAR_ID('Y','M','4','F') /* mode 4 (outlook) - forward range in meters (with SM4F) */
#define YMBN MAKE_FOURCHAR_ID('Y','M','B','N') /* Sonar enabled (0==DISABLED completely - with SMBN) */
#define YMLM MAKE_FOURCHAR_ID('Y','M','L','M') /* mode - auto/full/down/forward/outlook - with SMLM */
#define YSWR MAKE_FOURCHAR_ID('Y','S','W','R') /* software update ready from control head */
