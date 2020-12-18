#
# Copyright (c) 2020 Moddable Tech, Inc.
#
#   This file is part of the Moddable SDK Tools.
# 
#   The Moddable SDK Tools is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
# 
#   The Moddable SDK Tools is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
# 
#   You should have received a copy of the GNU General Public License
#   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
#
#   getPort.py: utility to determine what serial port idf.py will choose automatically if no ESPPORT is specified.
#

import sys
import io
sys.path.append(sys.argv[1])
from idf import get_default_serial_port
from io import BytesIO as StringIO
sys.stdout = StringIO()
port = get_default_serial_port()
sys.stdout = sys.__stdout__
print(port),