# Makefile for MinGW using the cygwin environment

# Run make in the src directory with the following command:
#  make -f ../windows/cygming.mak

CXX=g++
#CXXFLAGS=-g -Wall
CXXFLAGS=-g -mno-cygwin -Wall
#CXXFLAGS=-O2 -mno-cygwin -Wall

objects = 608.o \
  608_helpers.o \
  608_sami.o \
  608_srt.o \
  708.o \
  708_encoding.o \
  activity.o \
  asf_functions.o \
  avc_functions.o \
  bitstream.o \
  ccextractor.o \
  encoding.o \
  es_functions.o \
  es_userdata.o \
  file_functions.o \
  general_loop.o \
  mp4_bridge2bento4.o \
  myth.o \
  output.o \
  params.o \
  params_dump.o \
  sequencing.o \
  stream_functions.o \
  timing.o \
  ts_functions.o

bentoobjects = bento4/Ap48bdlAtom.o \
  bento4/Ap4AdtsParser.o \
  bento4/Ap4AesBlockCipher.o \
  bento4/Ap4Atom.o \
  bento4/Ap4AtomFactory.o \
  bento4/Ap4AtomSampleTable.o \
  bento4/Ap4AvcParser.o \
  bento4/Ap4AvccAtom.o \
  bento4/Ap4BitStream.o \
  bento4/Ap4ByteStream.o \
  bento4/Ap4Co64Atom.o \
  bento4/Ap4Command.o \
  bento4/Ap4CommandFactory.o \
  bento4/Ap4ContainerAtom.o \
  bento4/Ap4CttsAtom.o \
  bento4/Ap4DataBuffer.o \
  bento4/Ap4Debug.o \
  bento4/Ap4DecoderConfigDescriptor.o \
  bento4/Ap4DecoderSpecificInfoDescriptor.o \
  bento4/Ap4Descriptor.o \
  bento4/Ap4DescriptorFactory.o \
  bento4/Ap4DrefAtom.o \
  bento4/Ap4ElstAtom.o \
  bento4/Ap4EsDescriptor.o \
  bento4/Ap4EsdsAtom.o \
  bento4/Ap4Expandable.o \
  bento4/Ap4File.o \
  bento4/Ap4FileCopier.o \
  bento4/Ap4FileWriter.o \
  bento4/Ap4FragmentSampleTable.o \
  bento4/Ap4FrmaAtom.o \
  bento4/Ap4FtypAtom.o \
  bento4/Ap4GrpiAtom.o \
  bento4/Ap4HdlrAtom.o \
  bento4/Ap4HintTrackReader.o \
  bento4/Ap4Hmac.o \
  bento4/Ap4HmhdAtom.o \
  bento4/Ap4IkmsAtom.o \
  bento4/Ap4IodsAtom.o \
  bento4/Ap4Ipmp.o \
  bento4/Ap4IproAtom.o \
  bento4/Ap4IsfmAtom.o \
  bento4/Ap4IsltAtom.o \
  bento4/Ap4IsmaCryp.o \
  bento4/Ap4KeyWrap.o \
  bento4/Ap4LinearReader.o \
  bento4/Ap4Marlin.o \
  bento4/Ap4MdhdAtom.o \
  bento4/Ap4MehdAtom.o \
  bento4/Ap4MetaData.o \
  bento4/Ap4MfhdAtom.o \
  bento4/Ap4MfroAtom.o \
  bento4/Ap4MoovAtom.o \
  bento4/Ap4Movie.o \
  bento4/Ap4MovieFragment.o \
  bento4/Ap4Mp4AudioInfo.o \
  bento4/Ap4Mpeg2Ts.o \
  bento4/Ap4MvhdAtom.o \
  bento4/Ap4NmhdAtom.o \
  bento4/Ap4ObjectDescriptor.o \
  bento4/Ap4OdafAtom.o \
  bento4/Ap4OddaAtom.o \
  bento4/Ap4OdheAtom.o \
  bento4/Ap4OhdrAtom.o \
  bento4/Ap4OmaDcf.o \
  bento4/Ap4Piff.o \
  bento4/Ap4Processor.o \
  bento4/Ap4Protection.o \
  bento4/Ap4Results.o \
  bento4/Ap4RtpAtom.o \
  bento4/Ap4RtpHint.o \
  bento4/Ap4SLConfigDescriptor.o \
  bento4/Ap4Sample.o \
  bento4/Ap4SampleDescription.o \
  bento4/Ap4SampleEntry.o \
  bento4/Ap4SampleSource.o \
  bento4/Ap4SampleTable.o \
  bento4/Ap4SchmAtom.o \
  bento4/Ap4SdpAtom.o \
  bento4/Ap4SmhdAtom.o \
  bento4/Ap4StcoAtom.o \
  bento4/Ap4StdCFileByteStream.o \
  bento4/Ap4StreamCipher.o \
  bento4/Ap4String.o \
  bento4/Ap4StscAtom.o \
  bento4/Ap4StsdAtom.o \
  bento4/Ap4StssAtom.o \
  bento4/Ap4StszAtom.o \
  bento4/Ap4SttsAtom.o \
  bento4/Ap4SyntheticSampleTable.o \
  bento4/Ap4TfhdAtom.o \
  bento4/Ap4TfraAtom.o \
  bento4/Ap4TimsAtom.o \
  bento4/Ap4TkhdAtom.o \
  bento4/Ap4Track.o \
  bento4/Ap4TrakAtom.o \
  bento4/Ap4TrefTypeAtom.o \
  bento4/Ap4TrexAtom.o \
  bento4/Ap4TrunAtom.o \
  bento4/Ap4UrlAtom.o \
  bento4/Ap4Utils.o \
  bento4/Ap4UuidAtom.o \
  bento4/Ap4VmhdAtom.o \
  bento4/LinearReaderTest.o

bentoheaders = bento4/Ap4.h \
  bento4/Ap48bdlAtom.h \
  bento4/Ap4AdtsParser.h \
  bento4/Ap4AesBlockCipher.h \
  bento4/Ap4Array.h \
  bento4/Ap4Atom.h \
  bento4/Ap4AtomFactory.h \
  bento4/Ap4AtomSampleTable.h \
  bento4/Ap4AvcParser.h \
  bento4/Ap4AvccAtom.h \
  bento4/Ap4BitStream.h \
  bento4/Ap4ByteStream.h \
  bento4/Ap4Co64Atom.h \
  bento4/Ap4Command.h \
  bento4/Ap4CommandFactory.h \
  bento4/Ap4Config.h \
  bento4/Ap4Constants.h \
  bento4/Ap4ContainerAtom.h \
  bento4/Ap4CttsAtom.h \
  bento4/Ap4DataBuffer.h \
  bento4/Ap4Debug.h \
  bento4/Ap4DecoderConfigDescriptor.h \
  bento4/Ap4DecoderSpecificInfoDescriptor.h \
  bento4/Ap4Descriptor.h \
  bento4/Ap4DescriptorFactory.h \
  bento4/Ap4DrefAtom.h \
  bento4/Ap4DynamicCast.h \
  bento4/Ap4ElstAtom.h \
  bento4/Ap4EsDescriptor.h \
  bento4/Ap4EsdsAtom.h \
  bento4/Ap4Expandable.h \
  bento4/Ap4File.h \
  bento4/Ap4FileByteStream.h \
  bento4/Ap4FileCopier.h \
  bento4/Ap4FileWriter.h \
  bento4/Ap4FragmentSampleTable.h \
  bento4/Ap4FrmaAtom.h \
  bento4/Ap4FtypAtom.h \
  bento4/Ap4GrpiAtom.h \
  bento4/Ap4HdlrAtom.h \
  bento4/Ap4HintTrackReader.h \
  bento4/Ap4Hmac.h \
  bento4/Ap4HmhdAtom.h \
  bento4/Ap4IkmsAtom.h \
  bento4/Ap4Interfaces.h \
  bento4/Ap4IodsAtom.h \
  bento4/Ap4Ipmp.h \
  bento4/Ap4IproAtom.h \
  bento4/Ap4IsfmAtom.h \
  bento4/Ap4IsltAtom.h \
  bento4/Ap4IsmaCryp.h \
  bento4/Ap4KeyWrap.h \
  bento4/Ap4LinearReader.h \
  bento4/Ap4List.h \
  bento4/Ap4Marlin.h \
  bento4/Ap4MdhdAtom.h \
  bento4/Ap4MehdAtom.h \
  bento4/Ap4MetaData.h \
  bento4/Ap4MfhdAtom.h \
  bento4/Ap4MfroAtom.h \
  bento4/Ap4MoovAtom.h \
  bento4/Ap4Movie.h \
  bento4/Ap4MovieFragment.h \
  bento4/Ap4Mp4AudioInfo.h \
  bento4/Ap4Mpeg2Ts.h \
  bento4/Ap4MvhdAtom.h \
  bento4/Ap4NmhdAtom.h \
  bento4/Ap4ObjectDescriptor.h \
  bento4/Ap4OdafAtom.h \
  bento4/Ap4OddaAtom.h \
  bento4/Ap4OdheAtom.h \
  bento4/Ap4OhdrAtom.h \
  bento4/Ap4OmaDcf.h \
  bento4/Ap4Piff.h \
  bento4/Ap4Processor.h \
  bento4/Ap4Protection.h \
  bento4/Ap4Results.h \
  bento4/Ap4RtpAtom.h \
  bento4/Ap4RtpHint.h \
  bento4/Ap4SLConfigDescriptor.h \
  bento4/Ap4Sample.h \
  bento4/Ap4SampleDescription.h \
  bento4/Ap4SampleEntry.h \
  bento4/Ap4SampleSource.h \
  bento4/Ap4SampleTable.h \
  bento4/Ap4SchmAtom.h \
  bento4/Ap4SdpAtom.h \
  bento4/Ap4SmhdAtom.h \
  bento4/Ap4StcoAtom.h \
  bento4/Ap4StreamCipher.h \
  bento4/Ap4String.h \
  bento4/Ap4StscAtom.h \
  bento4/Ap4StsdAtom.h \
  bento4/Ap4StssAtom.h \
  bento4/Ap4StszAtom.h \
  bento4/Ap4SttsAtom.h \
  bento4/Ap4SyntheticSampleTable.h \
  bento4/Ap4TfhdAtom.h \
  bento4/Ap4TfraAtom.h \
  bento4/Ap4TimsAtom.h \
  bento4/Ap4TkhdAtom.h \
  bento4/Ap4Track.h \
  bento4/Ap4TrakAtom.h \
  bento4/Ap4TrefTypeAtom.h \
  bento4/Ap4TrexAtom.h \
  bento4/Ap4TrunAtom.h \
  bento4/Ap4Types.h \
  bento4/Ap4UrlAtom.h \
  bento4/Ap4Utils.h \
  bento4/Ap4UuidAtom.h \
  bento4/Ap4Version.h \
  bento4/Ap4VmhdAtom.h \

ccextractor : $(objects) $(bentoobjects)
	$(CXX) $(CXXFLAGS) -o $@ $(objects) $(bentoobjects)
	strip $@

$(objects) : ccextractor.h 608.h 708.h bitstream.h

# Lazy solution.
$(bentoobjects) : $(bentoheaders)

.PHONY : clean
clean :
	rm ccextractor.exe $(objects) $(bentoobjects)
