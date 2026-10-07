/* Phase33.2 unavailable-media boundary, NOT a compressed-media player.
 * No supported MIME/codec, player handle, decoded frame, callback or worker
 * is advertised. Invalid-handle calls have no successful operation to report.
 * Valid player lifecycle remains Phase33.3 and must replace this boundary.
 */
#include <stdbool.h>
#include "starboard/player.h"
extern void abort(void);
static void reject_foreign_player(SbPlayer player)
{if(player!=kSbPlayerInvalid)abort();}
static void reject_foreign_target(SbDecodeTarget target)
{if(target!=kSbDecodeTargetInvalid)abort();}
SbPlayer SbPlayerCreate(SbWindow window,const SbPlayerCreationParam *param,
 SbPlayerDeallocateSampleFunc deallocate,SbPlayerDecoderStatusFunc decoder,
 SbPlayerStatusFunc status,SbPlayerErrorFunc error,void *context,
 SbDecodeTargetGraphicsContextProvider *provider)
{
 (void)window;(void)param;(void)deallocate;(void)decoder;(void)status;
 (void)error;(void)context;(void)provider;
 /* Never retain callback/sample ownership on failed creation. */
 return kSbPlayerInvalid;
}
SbPlayerOutputMode SbPlayerGetPreferredOutputMode(const SbPlayerCreationParam *param)
{(void)param;return kSbPlayerOutputModeInvalid;}
void SbPlayerDestroy(SbPlayer p){reject_foreign_player(p);}
void SbPlayerSeek(SbPlayer p,int64_t timestamp,int ticket)
{(void)timestamp;(void)ticket;reject_foreign_player(p);}
void SbPlayerWriteSamples(SbPlayer p,SbMediaType type,const SbPlayerSampleInfo *samples,int count)
{(void)type;(void)samples;(void)count;reject_foreign_player(p);}
int SbPlayerGetMaximumNumberOfSamplesPerWrite(SbPlayer p,SbMediaType type)
{(void)type;reject_foreign_player(p);return 0;}
void SbPlayerWriteEndOfStream(SbPlayer p,SbMediaType type)
{(void)type;reject_foreign_player(p);}
void SbPlayerSetBounds(SbPlayer p,int z,int x,int y,int width,int height)
{(void)z;(void)x;(void)y;(void)width;(void)height;reject_foreign_player(p);}
bool SbPlayerSetPlaybackRate(SbPlayer p,double rate)
{(void)rate;reject_foreign_player(p);return false;}
void SbPlayerSetVolume(SbPlayer p,double volume)
{(void)volume;reject_foreign_player(p);}
void SbPlayerGetInfo(SbPlayer p,SbPlayerInfo *out)
{(void)out;reject_foreign_player(p);/* No fabricated playing state. */}
SbDecodeTarget SbPlayerGetCurrentFrame(SbPlayer p)
{reject_foreign_player(p);return kSbDecodeTargetInvalid;}
bool SbPlayerGetAudioConfiguration(SbPlayer p,int index,SbMediaAudioConfiguration *out)
{(void)index;(void)out;reject_foreign_player(p);return false;}
void SbDecodeTargetRelease(SbDecodeTarget target){reject_foreign_target(target);}
bool SbDecodeTargetGetInfo(SbDecodeTarget target,SbDecodeTargetInfo *out)
{(void)out;reject_foreign_target(target);return false;}
