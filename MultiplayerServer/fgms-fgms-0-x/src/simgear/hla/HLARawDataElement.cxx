// Copyright (C) 2009 - 2012  Mathias Froehlich - Mathias.Froehlich@web.de
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif

#include <simgear/compiler.h>

#include "HLARawDataElement.hxx"

namespace simgear {

HLARawDataElement::HLARawDataElement(const HLADataType* dataType) :
    _dataType(dataType)
{
}

HLARawDataElement::~HLARawDataElement()
{
}

bool
HLARawDataElement::encode(HLAEncodeStream& stream) const
{
    stream.setData(_rtiData);
    stream.skip(_rtiData.size());
    return true;
}

bool
HLARawDataElement::decode(HLADecodeStream& stream)
{
    _rtiData = stream.getData();
    stream.skip(_rtiData.size());
    return true;
}

const HLADataType*
HLARawDataElement::getDataType() const
{
    return _dataType.get();
}

bool
HLARawDataElement::setDataType(const HLADataType* dataType)
{
    _dataType = dataType;
    return true;
}

}