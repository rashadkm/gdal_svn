
include ../GDALmake.opt

OBJ	=	gnmnetwork.o gnmgdalnetwork.o gnmgdalrule.o gnmmanager.o

CPPFLAGS	:=	$(GDAL_INCLUDE)	$(CPPFLAGS) -I../ogr -I../ogr/ogrsf_frmts -I../gcore -I./gnm/frmts -I../gnm

default:	analysis-target $(OBJ:.o=.$(OBJ_EXT))

analysis-target:
	(cd analysis; $(MAKE))

$(OBJ):	gnm.h gnm_frmts.h gnm_api.h

obj:	$(OBJ)

clean:
	$(RM) *.o $(OBJ)
	(cd analysis; $(MAKE) clean)

docs:
	(cd ..; $(MAKE) docs)

install:
	for f in *.h ; do $(INSTALL_DATA) $$f $(DESTDIR)$(INST_INCLUDE) ; done
