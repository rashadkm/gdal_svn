
include ../../GDALmake.opt

OBJ	=	gnmstdanalysis.o
OUT_OBJ =	$(foreach file,$(OBJ),../$(file))

../%.$(OBJ_EXT):	%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

../%.$(OBJ_EXT):	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

%.$(OBJ_EXT):	%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

%.$(OBJ_EXT):	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
	

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS) -I../

default:	$(OUT_OBJ:.o=.$(OBJ_EXT))

$(OBJ):	gnmstdanalysis.h


docs:
	(cd ..; $(MAKE) docs)

install:
	for f in *.h ; do $(INSTALL_DATA) $$f $(DESTDIR)$(INST_INCLUDE) ; done

