#include "capture-output.hpp"
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <chrono>
#include <thread>
#include <iostream>
#include <util/platform.h>

int main(int argc,char** argv) {
 QGuiApplication app(argc,argv);
 if(argc<3){std::cerr<<"usage: obs-capture-test OUTPUT SECONDS\n";return 2;}
 const int seconds=QString(argv[2]).toInt();
 if(seconds<1 || seconds>9000)return 2;
 const QString out=QDir::cleanPath(QString::fromLocal8Bit(argv[1]));
 if(QFile::exists(out)) {std::cerr<<"output must be new\n";return 2;}
 if(!obs_startup("en-US",nullptr,nullptr))return 3;
 obs_add_data_path("C:/Program Files/obs-studio/data/libobs/");
 obs_video_info vi{};vi.graphics_module="C:/Program Files/obs-studio/bin/64bit/libobs-d3d11.dll";vi.adapter=0;
 vi.base_width=vi.output_width=1920;vi.base_height=vi.output_height=1080;
 vi.fps_num=30000;vi.fps_den=1001;vi.output_format=VIDEO_FORMAT_NV12;
 vi.colorspace=VIDEO_CS_709;vi.range=VIDEO_RANGE_PARTIAL;vi.gpu_conversion=true;
 vi.scale_type=OBS_SCALE_BICUBIC;
 if(obs_reset_video(&vi)!=OBS_VIDEO_SUCCESS)return 4;
 obs_audio_info ai{48000,SPEAKERS_STEREO};if(!obs_reset_audio(&ai))return 5;
 for(const char* name:{"obs-nvenc","obs-ffmpeg","obs-filters"}) {
  obs_module_t* mod=nullptr;
  const auto dll=QString("C:/Program Files/obs-studio/obs-plugins/64bit/%1.dll").arg(name).toUtf8();
  const auto data=QString("C:/Program Files/obs-studio/data/obs-plugins/%1").arg(name).toUtf8();
  if(obs_open_module(&mod,dll.constData(),data.constData())==MODULE_SUCCESS)obs_init_module(mod);
 }
 obs_post_load_modules();
 obs_source_info si{};si.id="hhc_synthetic_test";si.type=OBS_SOURCE_TYPE_INPUT;
 si.output_flags=OBS_SOURCE_ASYNC_VIDEO|OBS_SOURCE_AUDIO;
 si.get_name=[](void*){return "HHC synthetic test";};
 si.create=[](obs_data_t*,obs_source_t* s)->void*{return s;};si.destroy=[](void*){};
 obs_register_source(&si);
 auto* source=obs_source_create_private(si.id,"Synthetic timecode",nullptr);
 auto* scene=obs_scene_create_private("Synthetic Program");obs_scene_add(scene,source);
 obs_set_output_source(0,obs_scene_get_source(scene));
 obs_source_set_audio_mixers(source,1);
 hhc::CaptureOutput::registerOutput();
 int result=0;
 {
  hhc::CaptureOutput capture;
  if(!capture.start({out,1})) {std::cerr<<"FAIL native output start: "<<capture.error().toStdString()<<'\n';result=6;}
  else {
   QImage image(1920,1080,QImage::Format_RGBA8888);
   std::array<float,1600> audio{};
   const auto start=std::chrono::steady_clock::now();
   const uint64_t base=os_gettime_ns();
   for(int frame=0;frame<seconds*30 && capture.active();++frame){
    image.fill(QColor::fromHsv((frame/30)%360,180,90));
    QPainter painter(&image);painter.setPen(Qt::white);painter.setFont(QFont("Consolas",72));
    painter.drawText(100,200,QString("HHC SYNTHETIC / PROGRAM"));
    painter.drawText(100,400,QString("%1:%2:%3  frame %4").arg(frame/108000,2,10,QChar('0')).arg((frame/1800)%60,2,10,QChar('0')).arg((frame/30)%60,2,10,QChar('0')).arg(frame));
    painter.drawRect((frame*7)%1600,600,200,200);painter.end();
    obs_source_frame vf{};vf.data[0]=image.bits();vf.linesize[0]=image.bytesPerLine();vf.width=1920;vf.height=1080;vf.format=VIDEO_FORMAT_RGBA;vf.timestamp=base+uint64_t(frame)*1000000000ULL/30;vf.full_range=true;
    obs_source_output_video(source,&vf);
    obs_source_audio af{};af.data[0]=reinterpret_cast<const uint8_t*>(audio.data());af.data[1]=af.data[0];af.frames=1600;af.speakers=SPEAKERS_STEREO;af.format=AUDIO_FORMAT_FLOAT_PLANAR;af.samples_per_sec=48000;af.timestamp=vf.timestamp;
    obs_source_output_audio(source,&af);
    std::this_thread::sleep_until(start+std::chrono::nanoseconds(uint64_t(frame+1)*1000000000ULL/30));
   }
   capture.stop(hhc::StopReason::User);
   if(!capture.wait(30000)){std::cerr<<"FAIL output finalization "<<capture.error().toStdString()<<'\n';result=7;}
   for(const char* rendition:{"1080p","720p","480p"}) {
    QFile playlist(out+"/"+rendition+"/index.m3u8");
    double total=0;int count=0;
    if(playlist.open(QIODevice::ReadOnly)){const auto text=QString::fromUtf8(playlist.readAll());auto matches=QRegularExpression("#EXTINF:([0-9.]+)").globalMatch(text);while(matches.hasNext()){total+=matches.next().captured(1).toDouble();++count;}}
    if(total<seconds-0.5 || total>seconds+0.1 || (seconds==61 && count!=3)){std::cerr<<"FAIL timeline "<<rendition<<" duration="<<total<<" segments="<<count<<"\\n";result=10;}
   }
   QFile inventory(out+"/inventory.json");
   if(!inventory.open(QIODevice::ReadOnly)){std::cerr<<"FAIL missing inventory\n";result=8;}
   else {auto j=QJsonDocument::fromJson(inventory.readAll()).object();if(!j["normalEnd"].toBool()){std::cerr<<"FAIL abnormal end\n";result=9;}}
  }
 }
 obs_set_output_source(0,nullptr);obs_scene_release(scene);obs_source_release(source);obs_shutdown();
 return result;
}



