#!/usr/bin/gawk -f

  # Copyright (c) 1997-2016 Victor Lavrenko (v.lavrenko@gmail.com)
  
  # This file is part of YARI.
  
  # YARI is free software: you can redistribute it and/or modify it
  # under the terms of the GNU General Public License as published by
  # the Free Software Foundation, either version 3 of the License, or
  # (at your option) any later version.
  
  # YARI is distributed in the hope that it will be useful, but WITHOUT
  # ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  # or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  # License for more details.
  
  # You should have received a copy of the GNU General Public License
  # along with YARI. If not, see <http://www.gnu.org/licenses/>.
  

{
    ap = rel = 0; 
    for (ret=3; ret<NF; ++ret) ap += ++rel / $ret; 
    ap /= rel; 
    map += ap; 
    ++qrys; 
}

END { printf "MAP: %.4f over %3d qrys in %s\n", (map / qrys), qrys, FILENAME }
