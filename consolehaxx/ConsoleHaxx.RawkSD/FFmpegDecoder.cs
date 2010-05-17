﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using Tao.FFmpeg;
using System.IO;
using ConsoleHaxx.Common;
using System.Runtime.InteropServices;

namespace ConsoleHaxx.RawkSD
{
	public class FFmpegDecoder : IDecoder
	{
		public const int BufferSize = RawkAudio.Decoder.BufferSize;
		public static bool Enabled = false;

		public JaggedShortArray AudioBuffer { get; protected set; }
		public int Channels { get; protected set; }
		public int SampleRate { get; protected set; }
		public long Samples { get; protected set; }

		protected string Filename;
		protected bool Disposed;
		
		protected IntPtr FormatPointer;
		protected IntPtr DecoderPointer;
		protected IntPtr PacketPointer;
		protected IntPtr FFmpegBuffer;
		
		protected FFmpeg.AVFormatContext Format;
		protected FFmpeg.AVCodecContext Codec;
		protected FFmpeg.AVStream AVStream;
		
		protected int StreamIndex;
		protected int FFmpegBufferSize;

		public FFmpegDecoder(Stream stream)
		{
			if (!Enabled) {
				try {
					FFmpeg.av_register_all();
					Enabled = true;
				} catch (Exception ex) {
					throw ex;
				}
			}

			stream.Position = 0;
			Filename = Path.GetTempFileName();
			Stream filestream = new FileStream(Filename, FileMode.Create, FileAccess.Write);
			Util.StreamCopy(filestream, stream, stream.Length);
			filestream.Close();

			FFmpeg.av_open_input_file(out FormatPointer, Filename, IntPtr.Zero, 0, IntPtr.Zero);
			if (FFmpeg.av_find_stream_info(FormatPointer) < 0)
				throw new FormatException();
			Format = (FFmpeg.AVFormatContext)Marshal.PtrToStructure(FormatPointer, typeof(FFmpeg.AVFormatContext));
			for (StreamIndex = 0; StreamIndex < Format.nb_streams; StreamIndex++) {
				if (Format.streams[StreamIndex] == IntPtr.Zero)
					continue;
				AVStream = (FFmpeg.AVStream)Marshal.PtrToStructure(Format.streams[StreamIndex], typeof(FFmpeg.AVStream));
				Codec = (FFmpeg.AVCodecContext)Marshal.PtrToStructure(AVStream.codec, typeof(FFmpeg.AVCodecContext));
				if (Codec.codec_type == FFmpeg.CodecType.CODEC_TYPE_AUDIO) {
					DecoderPointer = FFmpeg.avcodec_find_decoder(Codec.codec_id);
					break;
				}
			}

			if (DecoderPointer == IntPtr.Zero)
				throw new FormatException();

			FFmpeg.avcodec_open(AVStream.codec, DecoderPointer);

			Channels = Codec.channels;
			SampleRate = Codec.sample_rate;
			Samples = AVStream.time_base.num * AVStream.duration * SampleRate / AVStream.time_base.den;

			PacketPointer = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(FFmpeg.AVPacket)));
			//FFmpegBufferSize = (FFmpeg.AVCODEC_MAX_AUDIO_FRAME_SIZE * 3) / 2;
			FFmpegBufferSize = BufferSize * Channels * 2;
			FFmpegBuffer = Marshal.AllocHGlobal(FFmpegBufferSize);
			AudioBuffer = new JaggedShortArray(Channels, BufferSize);
		}

		~FFmpegDecoder()
		{
			Dispose();
		}

		public int Read(int count)
		{
			int offset = 0;

			short[] bitstream = new short[FFmpegBufferSize / 2];

			while (count - offset >= FFmpegBufferSize / 2 / Channels) {
				if (FFmpeg.av_read_frame(FormatPointer, PacketPointer) < 0)
					break;
				FFmpeg.AVPacket packet = (FFmpeg.AVPacket)Marshal.PtrToStructure(PacketPointer, typeof(FFmpeg.AVPacket));

				while (packet.size > 0) {
					int datasize = FFmpegBufferSize;
					int used = FFmpeg.avcodec_decode_audio2(AVStream.codec, FFmpegBuffer, ref datasize, packet.data, packet.size);
					packet.size -= used;
					packet.data = new IntPtr(packet.data.ToInt32() + used);

					if (datasize <= 0)
						break;

					int samples = datasize / 2 / Channels;
					Marshal.Copy(FFmpegBuffer, bitstream, 0, datasize / 2);
					AudioBuffer.DeinterlaceFrom(bitstream, samples, offset);
					offset += samples;
				}
			}

			return offset;
		}

		public int Read()
		{
			return Read(BufferSize);
		}

		public void Seek(long sample)
		{
			FFmpeg.av_seek_frame(FormatPointer, StreamIndex, AVStream.time_base.num * sample / AVStream.time_base.den / SampleRate, FFmpeg.AVSEEK_FLAG_ANY);
		}

		public void Dispose()
		{
			if (Disposed)
				return;

			Marshal.FreeHGlobal(PacketPointer);

			FFmpeg.avcodec_close(AVStream.codec);
			FFmpeg.av_close_input_file(FormatPointer);

			File.Delete(Filename);

			Disposed = true;
		}
	}
}
