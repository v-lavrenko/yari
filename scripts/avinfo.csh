#!/usr/bin/env tcsh

foreach f ($argv)
ffprobe -show_streams $f |& gawk '/pix_fmt/ || /codec_name/ || /nb_frames/ || /codec_tag_string/ || /^width/ || /^height/ || /r_frame_rate/' | cut -d= -f2 | tr '\n' '\t'
echo $f
end
