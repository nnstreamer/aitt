/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.samsung.android.modules.webrtc;

import static org.webrtc.SessionDescription.Type.ANSWER;
import static java.lang.Math.max;
import static java.lang.Math.toIntExact;

import android.content.Context;
import android.util.Log;

import com.github.luben.zstd.Zstd;

import org.json.JSONException;
import org.json.JSONObject;
import org.webrtc.DataChannel;
import org.webrtc.IceCandidate;
import org.webrtc.MediaConstraints;
import org.webrtc.MediaStream;
import org.webrtc.MediaStreamTrack;
import org.webrtc.PeerConnection;
import org.webrtc.RtpReceiver;
import org.webrtc.SessionDescription;
import org.webrtc.VideoFrame;
import org.webrtc.VideoSink;
import org.webrtc.VideoTrack;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

public final class WebRTCSubscriber extends WebRTC {

   private static final String TAG = "WebRTCSubscriber";

   private final ByteArrayOutputStream baos;
   private boolean recvLargeChunk = false;
   private int frameHeight = 0;
   private int frameWidth = 0;

   public WebRTCSubscriber(Context appContext) throws InstantiationException {
      super(appContext);
      baos = new ByteArrayOutputStream();
   }

   @Override
   public void setRemoteDescription(String sdp) {
      if (sdp == null || sdp.isEmpty())
         return;

      Log.d(TAG, "Got answer " + sdp);
      String answer = extractSDP(sdp);
      peerConnection.setRemoteDescription(new SimpleSdpObserver(), new SessionDescription(ANSWER, answer));
   }

   @Override
   public boolean setPeerDiscoveryId(String id) {
      if (peerDiscoveryId != null && !peerDiscoveryId.isEmpty()) {
         Log.e(TAG, "Stream peer already added");
         return false;
      }
      this.peerDiscoveryId = id;
      createOffer();
      return true;
   }

   @Override
   public int getFrameHeight() {
      return frameHeight;
   }

   @Override
   public int getFrameWidth() {
      return frameWidth;
   }

   @Override
   protected void initializePeerConnection() {
      PeerConnection.RTCConfiguration rtcConfig = new PeerConnection.RTCConfiguration(new ArrayList<>());

      PeerConnection.Observer pcObserver = new PeerConnection.Observer() {
         @Override
         public void onSignalingChange(PeerConnection.SignalingState signalingState) {
            Log.d(TAG, "onSignalingChange: ");
         }

         @Override
         public void onIceConnectionChange(PeerConnection.IceConnectionState iceConnectionState) {
            Log.d(TAG, "onIceConnectionChange: ");
         }

         @Override
         public void onIceConnectionReceivingChange(boolean b) {
            Log.d(TAG, "onIceConnectionReceivingChange: ");
         }

         @Override
         public void onIceGatheringChange(PeerConnection.IceGatheringState iceGatheringState) {
            Log.d(TAG, "onIceGatheringChange: ");
         }

         @Override
         public void onIceCandidate(IceCandidate iceCandidate) {
            Log.d(TAG, "onIceCandidate: " + iceCandidate.sdp);

            try {
               JSONObject candidateMessge = new JSONObject();
               candidateMessge.put("candidate", iceCandidate.sdp);
               candidateMessge.put("sdpMLineIndex", iceCandidate.sdpMLineIndex);

               JSONObject iceMessage = new JSONObject();
               iceMessage.put("ice", candidateMessge);
               iceCandidates.add(iceMessage.toString());

               if (iceCandidateAddedCallback != null)
                  iceCandidateAddedCallback.onIceCandidate();
            } catch (JSONException e) {
               Log.e(TAG, "Failed to set ice candidate: " + e.getMessage());
            }
         }

         @Override
         public void onIceCandidatesRemoved(IceCandidate[] iceCandidates) {
            Log.d(TAG, "onIceCandidatesRemoved: ");
         }

         @Override
         public void onAddStream(MediaStream mediaStream) {
            Log.d(TAG, "onAddStream: " + mediaStream.videoTracks.size());
            VideoTrack remoteVideoTrack = mediaStream.videoTracks.get(0);
            remoteVideoTrack.setEnabled(true);
            // ToDo : update stream state as playing
         }

         @Override
         public void onRemoveStream(MediaStream mediaStream) {
            Log.d(TAG, "onRemoveStream: ");
         }

         @Override
         public void onDataChannel(DataChannel dataChannel) {
            Log.d(TAG, "onDataChannel: ");
            dataChannel.registerObserver(new DataChannel.Observer() {
               @Override
               public void onBufferedAmountChange(long l) {
                  //Keep this callback for future usage
                  Log.d(TAG, "onBufferedAmountChange:");
               }

               @Override
               public void onStateChange() {
                  Log.d(TAG, "onStateChange: remote data channel state: " + dataChannel.state().toString());
               }

               @Override
               public void onMessage(DataChannel.Buffer buffer) {
                  Log.d(TAG, "onMessage: got message");
                  if (!recvLargeChunk && buffer.data.capacity() < MAX_MESSAGE_SIZE) {
                     byte[] array = new byte[buffer.data.capacity()];
                     buffer.data.rewind();
                     buffer.data.get(array);
                     dataCallback.pushData(array);
                  } else {
                     String message = StandardCharsets.UTF_8.decode(buffer.data).toString();
                     handleLargeMessage(message, buffer);
                  }
               }
            });
         }

         @Override
         public void onRenegotiationNeeded() {
            Log.d(TAG, "onRenegotiationNeeded: ");
         }

         @Override
         public void onAddTrack(RtpReceiver rtpReceiver, MediaStream[] mediaStreams) {
            MediaStreamTrack track = rtpReceiver.track();
            if (track instanceof VideoTrack) {
               Log.i(TAG, "onAddTrack");
               VideoTrack remoteVideoTrack = (VideoTrack) track;
               remoteVideoTrack.setEnabled(true);
               ProxyVideoSink videoSink = new ProxyVideoSink();
               remoteVideoTrack.addSink(videoSink);
            }
         }
      };

      peerConnection = connectionFactory.createPeerConnection(rtcConfig, pcObserver);
      if (peerConnection == null)
         return;

      localDataChannel = peerConnection.createDataChannel("DataChannel", new DataChannel.Init());
   }

   private void createOffer() {
      MediaConstraints sdpMediaConstraints = new MediaConstraints();
      sdpMediaConstraints.mandatory.add(new MediaConstraints.KeyValuePair("OfferToReceiveVideo", "true"));

      peerConnection.createOffer(new SimpleSdpObserver() {
         @Override
         public void onCreateSuccess(SessionDescription sessionDescription) {
            Log.d(TAG, "CreateOffer : onCreateSuccess");
            peerConnection.setLocalDescription(new SimpleSdpObserver(), sessionDescription);

            try {
               JSONObject sdpMessage = new JSONObject();
               sdpMessage.put("type", "offer");
               sdpMessage.put("sdp", sessionDescription.description);

               JSONObject offerMessage = new JSONObject();
               offerMessage.put("sdp", sdpMessage);
               localDescription = offerMessage.toString();
            } catch (JSONException e) {
               Log.e(TAG, "Failed to set local description: " + e.getMessage());
            }
         }
      }, sdpMediaConstraints);
   }

   private void handleLargeMessage(String message, DataChannel.Buffer buffer) {
      if (EOF_MESSAGE.equals(message)) {
         // Received data is in compressed format, un compress it
         try {
            byte[] compBytes = baos.toByteArray();
            long deCompSize = Zstd.decompressedSize(compBytes);
            byte[] deCompBytes = Zstd.decompress(compBytes, max(toIntExact(deCompSize), MAX_UNCOMPRESSED_MESSAGE_SIZE));
            Log.d(TAG, "UnCompressed Byte array size: " + deCompBytes.length);
            dataCallback.pushData(deCompBytes);
            baos.reset();
            recvLargeChunk = false;
         } catch (Exception e) {
            Log.e(TAG, "Error during UnCompression: ");
         }
      } else {
         recvLargeChunk = true;
         try {
            byte[] array = new byte[buffer.data.capacity()];
            buffer.data.rewind();
            buffer.data.get(array);
            baos.write(array);
         } catch (IOException e) {
            Log.e(TAG, "Failed to write to byteArrayOutputStream " + e);
         }
      }
   }

   /**
    * Class to create proxy video sink
    */
   private class ProxyVideoSink implements VideoSink {

      /**
       * Method to send data through data call back
       *
       * @param frame VideoFrame to be transferred using media channel
       */
      @Override
      synchronized public void onFrame(VideoFrame frame) {
         byte[] nv21Frame = createNV21Data(frame.getBuffer().toI420());
         frameHeight = frame.getBuffer().getHeight();
         frameWidth = frame.getBuffer().getWidth();
         dataCallback.pushData(nv21Frame);
      }

      /**
       * Method used to convert VideoFrame to NV21 data format
       *
       * @param i420Buffer VideoFrame in I420 buffer format
       * @return the video frame in NV21 data format
       */
      public byte[] createNV21Data(VideoFrame.I420Buffer i420Buffer) {
         final int width = i420Buffer.getWidth();
         final int height = i420Buffer.getHeight();
         final int chromaWidth = (width + 1) / 2;
         final int chromaHeight = (height + 1) / 2;
         final int ySize = width * height;
         final byte[] nv21Data = new byte[ySize + width * chromaHeight];
         byte[] i420ByteArrayU = new byte[i420Buffer.getDataU().remaining()];
         byte[] i420ByteArrayV = new byte[i420Buffer.getDataV().remaining()];
         //Copy i420 Y bytebuffer to nv21Data.
         i420Buffer.getDataY().get(nv21Data, 0, i420Buffer.getDataY().remaining());
         //Copy i420 U bytebuffer to bytearrayU.
         i420Buffer.getDataU().get(i420ByteArrayU, 0, i420ByteArrayU.length);
         //Copy i420 V bytebuffer to bytearrayV.
         i420Buffer.getDataV().get(i420ByteArrayV, 0, i420ByteArrayV.length);
         int strideU = i420Buffer.getStrideU();
         int strideV = i420Buffer.getStrideV();
         //Y pixels are copied, just copy UV pixels to nv21data
         for (int y = 0; y < chromaHeight; ++y) {
            for (int x = 0; x < chromaWidth; ++x) {
               final byte uValue = i420ByteArrayU[y * strideU + x];
               final byte vValue = i420ByteArrayV[y * strideV + x];
               nv21Data[ySize + y * width + 2 * x] = vValue;
               nv21Data[ySize + y * width + 2 * x + 1] = uValue;
            }
         }
         return nv21Data;
      }
   }
}
