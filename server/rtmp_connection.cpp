#include "rtmp_connection.h"
#include <string.h>
#include <memory.h>
#include <unistd.h>
#include <stdlib.h>

namespace server
{
  static const AVal av_dquote = AVC("\"");
  static const AVal av_escdquote = AVC("\\\"");
  #define STR2AVAL(av,str)    av.av_val = (char*)str; av.av_len = strlen(av.av_val)

  #ifdef _DEBUG
  uint32_t debugTS = 0;

  int pnum = 0;

   FILE *netstackdump = NULL;
   FILE *netstackdump_read = NULL;
  #endif

  #define SAVC(x) static const AVal av_##x = AVC(#x)

  SAVC(app);
  SAVC(connect);
  SAVC(flashVer);
  SAVC(swfUrl);
  SAVC(pageUrl);
  SAVC(tcUrl);
  SAVC(fpad);
  SAVC(capabilities);
  SAVC(audioCodecs);
  SAVC(videoCodecs);
  SAVC(videoFunction);
  SAVC(objectEncoding);
  SAVC(_result);
  SAVC(createStream);
  SAVC(getStreamLength);
  SAVC(play);
  SAVC(fmsVer);
  SAVC(mode);
  SAVC(level);
  SAVC(code);
  SAVC(description);
  SAVC(secureToken);
  SAVC(onStatus);
  SAVC(status);
  SAVC(details);
  SAVC(clientid);
  SAVC(anchorid);

  static const AVal av_NetStream_Play_Start = AVC("NetStream.Play.Start");
  static const AVal av_Started_playing = AVC("Started playing");
  static const AVal av_NetStream_Play_Stop = AVC("NetStream.Play.Stop");
  static const AVal av_Stopped_playing = AVC("Stopped playing");
  static const AVal av_NetStream_Authenticate_UsherToken = AVC("NetStream.Authenticate.UsherToken");
  static const AVal av_publish = AVC("publish");
  static const AVal av_unpublish = AVC("FCUnpublish");
  void
  AVreplace(AVal *src, const AVal *orig, const AVal *repl)
  {
    char *srcbeg = src->av_val;
    char *srcend = src->av_val + src->av_len;
    char *dest, *sptr, *dptr;
    int n = 0;

    /* count occurrences of orig in src */
    sptr = src->av_val;
    while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
    {
      n++;
      sptr += orig->av_len;
    }
    if (!n)
        return;

    dest = (char*)malloc(src->av_len + 1 + (repl->av_len - orig->av_len) * n);

    sptr = src->av_val;
    dptr = dest;
    while (sptr < srcend && (sptr = strstr(sptr, orig->av_val)))
    {
        n = sptr - srcbeg;
        memcpy(dptr, srcbeg, n);
        dptr += n;
        memcpy(dptr, repl->av_val, repl->av_len);
        dptr += repl->av_len;
        sptr += orig->av_len;
        srcbeg = sptr;
    }
    n = srcend - srcbeg;
    memcpy(dptr, srcbeg, n);
    dptr += n;
    *dptr = '\0';
    src->av_val = dest;
    src->av_len = dptr - dest;
  }
  int rtmp_connection::send_connect_result(RTMP *r, double txn)
  {
    printf( "send_connect_result\n" );
    RTMPPacket packet;
    char pbuf[384], *pend = pbuf+sizeof(pbuf);
    AMFObject obj;
    AMFObjectProperty p, op;
    AVal av;

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "FMS/3,5,1,525");
    enc = AMF_EncodeNamedString(enc, pend, &av_fmsVer, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_capabilities, 31.0);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_mode, 1.0);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    *enc++ = AMF_OBJECT;

    STR2AVAL(av, "status");
    enc = AMF_EncodeNamedString(enc, pend, &av_level, &av);
    STR2AVAL(av, "NetConnection.Connect.Success");
    enc = AMF_EncodeNamedString(enc, pend, &av_code, &av);
    STR2AVAL(av, "Connection succeeded.");
    enc = AMF_EncodeNamedString(enc, pend, &av_description, &av);
    enc = AMF_EncodeNamedNumber(enc, pend, &av_objectEncoding, r->m_fEncoding);
    #if 0
    STR2AVAL(av, "58656322c972d6cdf2d776167575045f8484ea888e31c086f7b5ffbd0baec55ce442c2fb");
    enc = AMF_EncodeNamedString(enc, pend, &av_secureToken, &av);
    #endif
    STR2AVAL(p.p_name, "version");
    STR2AVAL(p.p_vu.p_aval, "3,5,1,525");
    p.p_type = AMF_STRING;
    obj.o_num = 1;
    obj.o_props = &p;
    op.p_type = AMF_OBJECT;
    STR2AVAL(op.p_name, "data");
    op.p_vu.p_object = obj;
    enc = AMFProp_Encode(&op, enc, pend);
    *enc++ = 0;
    *enc++ = 0;
    *enc++ = AMF_OBJECT_END;

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
  }

  int rtmp_connection::send_result_number( RTMP *r, double txn, double ID )
  {
    printf( "send_result_number\n" );
    RTMPPacket packet;
    char pbuf[256], *pend = pbuf+sizeof(pbuf);

    packet.m_nChannel = 0x03;     // control channel (invoke)
    packet.m_headerType = 1; /* RTMP_PACKET_SIZE_MEDIUM; */
    packet.m_packetType = RTMP_PACKET_TYPE_INVOKE;
    packet.m_nTimeStamp = 0;
    packet.m_nInfoField2 = 0;
    packet.m_hasAbsTimestamp = 0;
    packet.m_body = pbuf + RTMP_MAX_HEADER_SIZE;

    char *enc = packet.m_body;
    enc = AMF_EncodeString(enc, pend, &av__result);
    enc = AMF_EncodeNumber(enc, pend, txn);
    *enc++ = AMF_NULL;
    enc = AMF_EncodeNumber(enc, pend, ID);

    packet.m_nBodySize = enc - packet.m_body;

    return RTMP_SendPacket(r, &packet, FALSE);
  }


  rtmp_connection::rtmp_connection( rtmp_data_processor* processor, int log_level )
    :m_ssl_context( NULL ), m_processor( processor ),m_cur_package(0), m_last_package(0)
  {
    memset( &m_packet, 0, sizeof(RTMPPacket) );
    RTMP_debuglevel = (RTMP_LogLevel)log_level;
  }

  rtmp_connection::~rtmp_connection()
  {
    RTMPPacket_Free(&m_packet);
  }

  int rtmp_connection::handshake( int sockfd )
  {
    m_rtmp = RTMP_Alloc();
    RTMP_Init(m_rtmp);
    m_rtmp->m_sb.sb_socket = sockfd;
    
    if ( !m_cert.empty() && !m_key.empty() )
    {
       m_ssl_context = RTMP_TLS_AllocServerContext(m_cert.c_str(), m_key.c_str() );
    }

    if (m_ssl_context && !RTMP_TLS_Accept(m_rtmp, m_ssl_context))
    {
      RTMP_Log(RTMP_LOGERROR, "TLS handshake failed");
      return FAILED;
    }

    if (!RTMP_Serve(m_rtmp))
    {
      RTMP_Log(RTMP_LOGERROR, "Handshake failed");
      return FAILED;
    }

    return SUCCESS;
  }

  void rtmp_connection::cleanup()
  {
    RTMP_LogPrintf("Closing connection... ");
    RTMP_Close(m_rtmp);
      /* Should probably be done by RTMP_Close() ... */
    m_rtmp->Link.playpath.av_val = NULL;
    m_rtmp->Link.tcUrl.av_val = NULL;
    m_rtmp->Link.swfUrl.av_val = NULL;
    m_rtmp->Link.pageUrl.av_val = NULL;
    m_rtmp->Link.app.av_val = NULL;
    m_rtmp->Link.flashVer.av_val = NULL;
    if (m_rtmp->Link.usherToken.av_val)
    {
      free(m_rtmp->Link.usherToken.av_val);
      m_rtmp->Link.usherToken.av_val = NULL;
    }
    RTMP_Free(m_rtmp);
    RTMP_LogPrintf("done!\n\n");

    if ( m_ssl_context != NULL )
    {
      RTMP_TLS_FreeServerContext(m_ssl_context);
      m_ssl_context = NULL;
    }
  }

  int rtmp_connection::prepare()
  {
    if ( RTMP_IsConnected(m_rtmp) && RTMP_ReadPacket(m_rtmp, &m_packet))
    {
      if ( !RTMPPacket_IsReady(&m_packet) )
      {
        printf("it's not ready, continue \n");
        return CONTINUE;
      }
      return SUCCESS;
    }
    return FAILED;
  }


  int rtmp_connection::server_invoke( RTMPPacket* packet, int offset )
  {
    const char *body;
    unsigned int nBodySize;
    int ret = 0, nRes;

    body = packet->m_body + offset;
    nBodySize = packet->m_nBodySize - offset;

    if (body[0] != 0x02)      // make sure it is a string method name we start with
    {
      RTMP_Log(RTMP_LOGWARNING, "%s, Sanity failed. no string method in invoke packet",
        __FUNCTION__);
      return 0;
    }

    AMFObject obj;
    nRes = AMF_Decode(&obj, body, nBodySize, FALSE);
    if (nRes < 0)
    {
      RTMP_Log(RTMP_LOGERROR, "%s, error decoding invoke packet", __FUNCTION__);
      return 0;
    }

    AMF_Dump(&obj);
    AVal method;
    AMFProp_GetString(AMF_GetProp(&obj, NULL, 0), &method);
    double txn = AMFProp_GetNumber(AMF_GetProp(&obj, NULL, 1));
    RTMP_Log(RTMP_LOGDEBUG, "%s, client invoking <%s>", __FUNCTION__, method.av_val);

    RTMP_Log( RTMP_LOGDEBUG, "method %s and av_connect %s ", method.av_val, av_connect.av_val );
    if (AVMATCH(&method, &av_connect))
    {
      AMFObject cobj;
      AVal pname, pval;
      int i;

      //server->connect = packet->m_body;
      packet->m_body = NULL;

      AMFProp_GetObject(AMF_GetProp(&obj, NULL, 3), &cobj);
      for (i=0; i<cobj.o_num; i++)
      {
       pname = cobj.o_props[i].p_name;
       pval.av_val = NULL;
       pval.av_len = 0;
       if (cobj.o_props[i].p_type == AMF_STRING)
       {
         pval = cobj.o_props[i].p_vu.p_aval;
         //printf("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA%s-%s\n", pval.av_val, pname.av_val );
       }
       if (AVMATCH(&pname, &av_app))
       {
         m_rtmp->Link.app = pval;
         pval.av_val = NULL;
         if (!m_rtmp->Link.app.av_val)
           m_rtmp->Link.app.av_val = (char*)"";
         // server->arglen += 6 + pval.av_len;
         // server->argc += 2;
       }
       else if (AVMATCH(&pname, &av_flashVer))
       {
         m_rtmp->Link.flashVer = pval;
         pval.av_val = NULL;
         // server->arglen += 6 + pval.av_len;
         // server->argc += 2;
       }
       else if (AVMATCH(&pname, &av_swfUrl))
       {
         m_rtmp->Link.swfUrl = pval;
         pval.av_val = NULL;
         // server->arglen += 6 + pval.av_len;
         // server->argc += 2;
       }
       else if (AVMATCH(&pname, &av_tcUrl))
       {
         m_rtmp->Link.tcUrl = pval;
         pval.av_val = NULL;
         // server->arglen += 6 + pval.av_len;
         // server->argc += 2;
       }
       else if (AVMATCH(&pname, &av_pageUrl))
       {
         m_rtmp->Link.pageUrl = pval;
         pval.av_val = NULL;
         // server->arglen += 6 + pval.av_len;
         // server->argc += 2;
       }
       else if (AVMATCH(&pname, &av_audioCodecs))
       {
         m_rtmp->m_fAudioCodecs = cobj.o_props[i].p_vu.p_number;
       }
       else if (AVMATCH(&pname, &av_videoCodecs))
       {
         m_rtmp->m_fVideoCodecs = cobj.o_props[i].p_vu.p_number;
       }
       else if (AVMATCH(&pname, &av_objectEncoding))
       {
         m_rtmp->m_fEncoding = cobj.o_props[i].p_vu.p_number;
       }
       else if( AVMATCH(&pname, &av_anchorid ) )
       {
         m_id.assign(pval.av_val, pval.av_len);
       }
     }
          /* Still have more parameters? Copy them */
     if (obj.o_num > 3)
     {
       int i = obj.o_num - 3;
       m_rtmp->Link.extras.o_num = i;
       m_rtmp->Link.extras.o_props = (AMFObjectProperty*)malloc(i*sizeof(AMFObjectProperty));
       memcpy(m_rtmp->Link.extras.o_props, obj.o_props+3, i*sizeof(AMFObjectProperty));
       obj.o_num = 3;
       //server->arglen += countAMF(&m_rtmp->Link.extras, &server->argc);
     }
     send_connect_result(m_rtmp, txn);
    }
    else if (AVMATCH(&method, &av_createStream))
    {
      printf( "createStream %d\n", m_streamID );
      send_result_number(m_rtmp, txn, ++m_streamID);
    }
    else if (AVMATCH(&method, &av_getStreamLength))
    {
      send_result_number(m_rtmp, txn, 10.0);
    }
    else if (AVMATCH(&method, &av_NetStream_Authenticate_UsherToken))
    {
      AVal usherToken;
      AMFProp_GetString(AMF_GetProp(&obj, NULL, 3), &usherToken);
      AVreplace(&usherToken, &av_dquote, &av_escdquote);
      // server->arglen += 6 + usherToken.av_len;
      // server->argc += 2;
      m_rtmp->Link.usherToken = usherToken;
    }
    else if (AVMATCH(&method, &av_publish))
    {
      send_result_number( m_rtmp, txn, 10.0 );
      m_processor->initialize();
    }
    else if ( AVMATCH( &method, &av_unpublish ) )
    {
      m_processor->cleanup();
    }

    AMF_Reset(&obj);
    return ret;
  }

  int rtmp_connection::process()
  {
    int ret = 0;
    static bool done = false;
    RTMPPacket* packet = &m_packet;
    RTMP_Log(RTMP_LOGDEBUG, "%s, received packet type %02X, size %u bytes", __FUNCTION__,
      packet->m_packetType, packet->m_nBodySize);

    switch (packet->m_packetType)
    {
      case RTMP_PACKET_TYPE_CHUNK_SIZE:
  //      HandleChangeChunkSize(m_rtmp, packet);
      break;

      case RTMP_PACKET_TYPE_BYTES_READ_REPORT:
      break;

      case RTMP_PACKET_TYPE_CONTROL:
  //      HandleCtrl(m_rtmp, packet);
      break;

      case RTMP_PACKET_TYPE_SERVER_BW:
  //      HandleServerBW(m_rtmp, packet);
      break;

      case RTMP_PACKET_TYPE_CLIENT_BW:
   //     HandleClientBW(m_rtmp, packet);
      break;

      case RTMP_PACKET_TYPE_AUDIO:
        //RTMP_Log(RTMP_LOGDEBUG, "%s, received: audio %lu bytes, %d packets", __FUNCTION__, packet->m_nBodySize, ++packet_number);
        m_cur_package ++;
        if ( done )
        {
          m_last_package = m_cur_package;
          done = false;
        }

        if ( 2 == m_processor->process_audio( packet->m_body, packet->m_nBodySize ) )
        {
          done = true;
        }
        break;

      case RTMP_PACKET_TYPE_VIDEO:
        //RTMP_Log(RTMP_LOGDEBUG, "%s, received: video %lu bytes", __FUNCTION__, packet.m_nBodySize);
      break;

      case RTMP_PACKET_TYPE_FLEX_STREAM_SEND:
      break;

      case RTMP_PACKET_TYPE_FLEX_SHARED_OBJECT:
      break;

      case RTMP_PACKET_TYPE_FLEX_MESSAGE:
      {
        RTMP_Log(RTMP_LOGDEBUG, "%s, flex message, size %u bytes, not fully supported",
        __FUNCTION__, packet->m_nBodySize);
        if ( server_invoke( packet, 1 ) )
        {
         RTMP_Close(m_rtmp);
        }
        break;
       }
       case RTMP_PACKET_TYPE_INFO:
       break;

       case RTMP_PACKET_TYPE_SHARED_OBJECT:
       break;

       case RTMP_PACKET_TYPE_INVOKE:
       RTMP_Log( RTMP_LOGDEBUG, "%s, received: invoke %u bytes", __FUNCTION__, packet->m_nBodySize );
            //RTMP_LogHex(packet.m_body, packet.m_nBodySize);

       if ( server_invoke( packet, 0 ) )
         RTMP_Close( m_rtmp );
       break;

       case RTMP_PACKET_TYPE_FLASH_VIDEO:
       break;
       default:
       RTMP_Log(RTMP_LOGDEBUG, "%s, unknown packet type received: 0x%02x", __FUNCTION__, packet->m_packetType);
      #ifdef _DEBUG
       RTMP_LogHex(RTMP_LOGDEBUG, (uint8_t*)packet->m_body, packet->m_nBodySize);
      #endif
    }
    return ret;
  }

  rtmp_data_processor* rtmp_connection::get_data_processor() const
  {
    return m_processor;
  }

  std::string rtmp_connection::get_id() const
  {
    return m_id;
  }

  long rtmp_connection::get_last_package_num() const
  {
    return m_last_package;
  }

  long rtmp_connection::get_cur_package_num() const
  {
    return m_cur_package;
  }
}