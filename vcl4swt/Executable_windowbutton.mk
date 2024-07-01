# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#

$(eval $(call gb_Executable_Executable,windowbutton))

$(eval $(call gb_Executable_use_api,windowbutton,\
    offapi \
    udkapi \
))

$(eval $(call gb_Executable_set_include,windowbutton,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl4swt/inc \
))


$(eval $(call gb_Library_add_defs,windowbutton,\
    -DVCL_DLLIMPLEMENTATION \
    -DDLLIMPLEMENTATION_UITEST \
    -DCUI_DLL_NAME=\"$(call gb_Library_get_runtime_filename,$(call gb_Library__get_name,cui))\" \
    -DTK_DLL_NAME=\"$(call gb_Library_get_runtime_filename,$(call gb_Library__get_name,tk))\" \
    $(if $(SYSTEM_LIBFIXMATH),-DSYSTEM_LIBFIXMATH) \
))



$(eval $(call gb_Executable_use_libraries,windowbutton,\
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

$(eval $(call gb_Executable_add_exception_objects,windowbutton,\
    vcl4swt/src/test/windowbutton \
))

# vim: set noet sw=4 ts=4:
