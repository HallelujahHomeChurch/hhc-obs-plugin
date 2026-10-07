#include "capture-output.hpp"
#include <obs-frontend-api.h>
#include <obs-module.h>
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QImage>
#include <QPainter>
#include <QTimer>
#include <QJsonObject>
#include <atomic>
#include <thread>
#include <chrono>
#include <util/platform.h>
#include "hls-muxer.hpp"

// Developer fixture target only. Never compiled into the candidate plugin.
namespace {
std::unique_ptr<hhc::CaptureOutput> capture;
std::thread producer;
std::atomic<bool> running{false};
obs_source_t* source=nullptr;
obs_scene_t* scene=nullptr;
QString destination;
int duration=0;
void finish() {
 running=false;if(producer.joinable())producer.join();
 capture.reset();obs_frontend_set_current_scene(nullptr);
 if(scene){obs_scene_release(scene);scene=nullptr;}
 if(source){obs_source_release(source);source=nullptr;}
}
void begin() {
 const QString base=QCoreApplication::applicationDirPath()+"/../../";
 if(!QFileInfo::exists(base+"portable_mode.txt") || obs_frontend_streaming_active() || obs_frontend_recording_active()) {
  blog(LOG_ERROR,"[HHC fixture] Refusing non-portable or already active OBS");return;
 }
 destination=qEnvironmentVariable("HHC_FIXTURE_OUTPUT");duration=qEnvironmentVariableIntValue("HHC_FIXTURE_SECONDS");
 if(destination.isEmpty() || QFileInfo::exists(destination) || duration<1 || duration>9000)return;
 obs_source_info si{};si.id="hhc_fixture_synthetic";si.type=OBS_SOURCE_TYPE_INPUT;si.output_flags=OBS_SOURCE_ASYNC_VIDEO|OBS_SOURCE_AUDIO;
 si.get_name=[](void*){return "HHC synthetic timecode";};si.create=[](obs_data_t*,obs_source_t* s)->void*{return s;};si.destroy=[](void*){};obs_register_source(&si);
 source=obs_source_create_private(si.id,"HHC synthetic Program",nullptr);
 scene=obs_scene_create_private("HHC synthetic scene");obs_scene_add(scene,source);
 obs_frontend_set_current_scene(obs_scene_get_source(scene));obs_source_set_audio_mixers(source,1);
 running=true;
 producer=std::thread([]{
  QImage image(1920,1080,QImage::Format_RGBA8888);std::array<float,1602> samples{};
  const auto start=std::chrono::steady_clock::now();const auto ns=os_gettime_ns();
  uint64_t audioFrame=0;
  for(uint64_t frame=0;running;++frame){
   image.fill(QColor::fromHsv(static_cast<int>((frame/30)%360),170,100));
   QPainter p(&image);p.setPen(Qt::white);p.setFont(QFont("Consolas",64));
   p.drawText(90,180,"HHC SYNTHETIC / OBS PROGRAM");
   const auto ms=frame*1001/30;
   p.drawText(90,360,QString("%1:%2:%3.%4").arg(ms/3600000,2,10,QChar('0')).arg((ms/60000)%60,2,10,QChar('0')).arg((ms/1000)%60,2,10,QChar('0')).arg(ms%1000,3,10,QChar('0')));
   p.drawText(90,500,QString("30000/1001 fps | frame %1").arg(frame));p.drawRect(static_cast<int>((frame*7)%1600),650,200,200);p.end();
   obs_source_frame vf{};vf.data[0]=image.bits();vf.linesize[0]=image.bytesPerLine();vf.width=1920;vf.height=1080;vf.format=VIDEO_FORMAT_RGBA;vf.full_range=true;vf.timestamp=ns+frame*1001000000ULL/30;obs_source_output_video(source,&vf);
   const uint64_t nextAudio=(frame+1)*48000*1001/30000;
   obs_source_audio af{};af.data[0]=reinterpret_cast<const uint8_t*>(samples.data());af.data[1]=af.data[0];af.frames=static_cast<uint32_t>(nextAudio-audioFrame);af.speakers=SPEAKERS_STEREO;af.format=AUDIO_FORMAT_FLOAT_PLANAR;af.samples_per_sec=48000;af.timestamp=ns+audioFrame*1000000000ULL/48000;obs_source_output_audio(source,&af);audioFrame=nextAudio;
   std::this_thread::sleep_until(start+std::chrono::nanoseconds((frame+1)*1001000000ULL/30));
  }
 });
 QTimer::singleShot(1500,QCoreApplication::instance(),[]{
  capture=std::make_unique<hhc::CaptureOutput>();
  if(!capture->start({destination,1})){blog(LOG_ERROR,"[HHC fixture] %s",capture->error().toUtf8().constData());finish();return;}
  blog(LOG_INFO,"[HHC fixture] Started real OBS frontend synthetic capture for %d seconds",duration);
  QTimer::singleShot(duration*1000,QCoreApplication::instance(),[]{
   capture->stop(hhc::StopReason::User);
   auto* timer=new QTimer(QCoreApplication::instance());timer->setInterval(100);
   QObject::connect(timer,&QTimer::timeout,[timer]{
    if(capture->active())return;
    if(!capture->wait(1))return;
    timer->stop();timer->deleteLater();
    blog(LOG_INFO,"[HHC fixture] Capture complete");finish();
    QTimer::singleShot(500,QCoreApplication::instance(),[]{QCoreApplication::quit();});
   });timer->start();
  });
 });
}
void event(obs_frontend_event e,void*) {
 if(e==OBS_FRONTEND_EVENT_FINISHED_LOADING && qEnvironmentVariableIsSet("HHC_FIXTURE_OUTPUT"))QTimer::singleShot(1500,QCoreApplication::instance(),begin);
 if(e==OBS_FRONTEND_EVENT_EXIT){if(capture)capture->stop(hhc::StopReason::Shutdown);finish();}
}
}
void registerFixtureController(){obs_frontend_add_event_callback(event,nullptr);}
