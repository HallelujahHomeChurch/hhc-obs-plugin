#include "hls-muxer.hpp"
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <stdexcept>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
}
namespace hhc {
static void require(bool ok,const char* msg){if(!ok)throw std::runtime_error(msg);}
void atomicJson(const QString& path,const QJsonObject& j) {
 QSaveFile f(path);require(f.open(QIODevice::WriteOnly),"journal open failed");
 const auto bytes=QJsonDocument(j).toJson();
 require(f.write(bytes)==bytes.size() && f.commit(),"journal commit failed");
}
struct HlsMuxer::Impl {
 AVFormatContext* fmt=nullptr;
 QString root,relative,staging;
 QJsonArray inventory;
 QSet<QString> published;
 bool closed=false;
 ~Impl(){if(fmt){if(fmt->pb)avio_closep(&fmt->pb);avformat_free_context(fmt);}}
};
HlsMuxer::HlsMuxer(const QString& root,unsigned r,obs_encoder_t* video,obs_encoder_t* audio):d(std::make_unique<Impl>()) {
 d->root=root;d->relative=QString::number(profiles.at(r).height)+"p";
 d->staging=root+"/staging/"+d->relative;
 require(QDir().mkpath(d->staging) && QDir().mkpath(root+"/"+d->relative),"create mux directory failed");
 const auto playlist=(d->staging+"/index.m3u8").toUtf8();
 require(avformat_alloc_output_context2(&d->fmt,nullptr,"hls",playlist.constData())>=0,"allocate HLS failed");
 for(unsigned i=0;i<2;++i){
  auto* s=avformat_new_stream(d->fmt,nullptr);require(s!=nullptr,"allocate stream failed");
  auto* p=s->codecpar;p->codec_type=i?AVMEDIA_TYPE_AUDIO:AVMEDIA_TYPE_VIDEO;
  p->codec_id=i?AV_CODEC_ID_AAC:AV_CODEC_ID_H264;
  if(i){s->time_base={1,48000};p->sample_rate=48000;av_channel_layout_default(&p->ch_layout,2);p->bit_rate=128000;p->frame_size=1024;}
  else{s->time_base={1001,30000};s->avg_frame_rate={30000,1001};p->width=profiles[r].width;p->height=profiles[r].height;p->bit_rate=profiles[r].kbps*1000;p->format=AV_PIX_FMT_YUV420P;p->color_primaries=AVCOL_PRI_BT709;p->color_trc=AVCOL_TRC_BT709;p->color_space=AVCOL_SPC_BT709;p->color_range=AVCOL_RANGE_MPEG;}
  uint8_t* extra=nullptr;size_t size=0;
  require(obs_encoder_get_extra_data(i?audio:video,&extra,&size) && size>0,"encoder extra data missing");
  p->extradata=static_cast<uint8_t*>(av_mallocz(size+AV_INPUT_BUFFER_PADDING_SIZE));require(p->extradata!=nullptr,"extra data allocation failed");
  memcpy(p->extradata,extra,size);p->extradata_size=static_cast<int>(size);
 }
 AVDictionary* options=nullptr;
 av_dict_set(&options,"hls_time","30",0);
 av_dict_set(&options,"hls_segment_type","fmp4",0);
 av_dict_set(&options,"hls_playlist_type","vod",0);
 av_dict_set(&options,"hls_flags","temp_file+independent_segments",0);
 av_dict_set(&options,"hls_fmp4_init_filename","init.mp4",0);
 const auto pattern=(d->staging+"/segment-%05d.m4s").toUtf8();
 av_dict_set(&options,"hls_segment_filename",pattern.constData(),0);
 const int result=avformat_write_header(d->fmt,&options);av_dict_free(&options);
 require(result>=0,"HLS header failed");
}
HlsMuxer::~HlsMuxer()=default;
void HlsMuxer::write(const encoder_packet& p) {
 require(!d->closed,"mux already closed");
 AVPacket* packet=av_packet_alloc();require(packet!=nullptr,"packet allocation failed");
 if(av_new_packet(packet,static_cast<int>(p.size))<0){av_packet_free(&packet);throw std::runtime_error("packet bytes allocation failed");}
 memcpy(packet->data,p.data,p.size);packet->stream_index=p.type==OBS_ENCODER_VIDEO?0:1;
 packet->pts=p.pts;packet->dts=p.dts;packet->duration=p.type==OBS_ENCODER_VIDEO?p.timebase_num:1024;
 if(p.keyframe)packet->flags|=AV_PKT_FLAG_KEY;
 av_packet_rescale_ts(packet,{1,p.timebase_den},d->fmt->streams[packet->stream_index]->time_base);
 const int result=av_interleaved_write_frame(d->fmt,packet);av_packet_free(&packet);
 require(result>=0,"HLS packet write failed");publishClosed();
}
void HlsMuxer::publishClosed() {
 // FFmpeg temp_file renames only after closing a segment. Never read .tmp.
 const auto files=QDir(d->staging).entryList({"segment-*.m4s"},QDir::Files,QDir::Name);
 QStringList ready=files;
 if(!files.empty() || d->closed)ready.prepend("init.mp4");
 if(d->closed)ready.append("index.m3u8");
 for(const auto& name:ready){
  if(d->published.contains(name))continue;
  QFile source(d->staging+"/"+name);require(source.open(QIODevice::ReadOnly),"closed object missing");
  require(source.size()>0 && source.size()<=128LL*1024*1024,"object size limit");
  QSaveFile target(d->root+"/"+d->relative+"/"+name);require(target.open(QIODevice::WriteOnly),"queue object open failed");
  QCryptographicHash hash(QCryptographicHash::Sha256);
  while(!source.atEnd()) {const auto block=source.read(1024*1024);require(!block.isEmpty(),"object read failed");hash.addData(block);require(target.write(block)==block.size(),"object write failed");}
  require(target.commit(),"atomic object commit failed");
  d->inventory.append(QJsonObject{{"path",d->relative+"/"+name},{"size",source.size()},{"sha256",QString::fromLatin1(hash.result().toHex())}});
  d->published.insert(name);
  atomicJson(d->root+"/"+d->relative+"/closed.json",{{"objects",d->inventory}});
 }
}
void HlsMuxer::close(){require(!d->closed,"duplicate close");require(av_write_trailer(d->fmt)>=0,"HLS trailer failed");d->closed=true;publishClosed();}
QJsonArray HlsMuxer::objects() const{return d->inventory;}
}

