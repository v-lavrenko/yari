/*
  
  Copyright (c) 1997-2021 Victor Lavrenko (v.lavrenko@gmail.com)
  
  This file is part of YARI.
  
  YARI is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  YARI is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.
  
  You should have received a copy of the GNU General Public License
  along with YARI. If not, see <http://www.gnu.org/licenses/>.
  
*/

// http://ascii-table.com/ansi-escape-sequences.php
#define ESC        "\033["
#define CLS        ESC"2J"ESC"1;1H"
#define RESET      ESC"0m"
#define BOLD       ESC"1m"
#define NORMAL     ESC"2m"
#define UNDER      ESC"4m"
#define BLINK      ESC"5m"
#define INVERSE    ESC"7m"
#define fg_BLACK   ESC"30m"
#define fg_RED     ESC"31m"
#define fg_GREEN   ESC"32m"
#define fg_YELLOW  ESC"33m"
#define fg_BLUE    ESC"34m"
#define fg_MAGENTA ESC"35m"
#define fg_CYAN    ESC"36m"
#define fg_WHITE   ESC"37m"
#define bg_BLACK   ESC"40m"
#define bg_RED     ESC"41m"
#define bg_GREEN   ESC"42m"
#define bg_YELLOW  ESC"43m"
#define bg_BLUE    ESC"44m"
#define bg_MAGENTA ESC"45m"
#define bg_CYAN    ESC"46m"
#define bg_WHITE   ESC"47m"
