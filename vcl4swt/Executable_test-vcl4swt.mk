# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,test-vcl4swt))

$(eval $(call gb_Executable_use_api,test-vcl4swt,\
    offapi \
    udkapi \
))

$(eval $(call gb_Executable_set_include,test-vcl4swt,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl4swt/inc \
))

$(eval $(call gb_Executable_use_libraries,test-vcl4swt,\
    tl \
    sal \
    vcl \
    cppu \
    cppuhelper \
    comphelper \
    i18nlangtag \
    fwk \
    vcl4swt \
))

$(eval $(call gb_Executable_add_exception_objects,test-vcl4swt,\
    vcl4swt/src/test/test-vcl4swt \
))

# vim: set noet sw=4 ts=4:
