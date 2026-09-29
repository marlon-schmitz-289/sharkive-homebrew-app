FROM devkitpro/devkitarm:20260610

# makerom + bannertool for .cia builds (built from source: no arm64 binaries upstream)
RUN git clone https://github.com/3DSGuy/Project_CTR /tmp/ctr \
 && git -C /tmp/ctr checkout -q e8f5f529c54ff9b22a2491a480ffa69206bf7b19 \
 && make -C /tmp/ctr/makerom deps && make -C /tmp/ctr/makerom -j"$(nproc)" \
 && cp /tmp/ctr/makerom/bin/makerom /usr/local/bin/ \
 && git clone https://github.com/diasurgical/bannertool /tmp/bt \
 && git -C /tmp/bt checkout -q 16d8c5a0ce02a5e06e64ab42275132fca57c04a2 \
 && cd /tmp/bt \
 && gcc -O2 -c source/pc/stb_image.c source/pc/stb_vorbis.c \
 && g++ -O2 -Isource -DVERSION_MAJOR=1 -DVERSION_MINOR=2 -DVERSION_MICRO=0 source/*.cpp source/3ds/*.cpp source/pc/*.cpp stb_image.o stb_vorbis.o -o /usr/local/bin/bannertool \
 && rm -rf /tmp/ctr /tmp/bt

WORKDIR /src
