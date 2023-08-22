cache()
include(biblioteq-source.pro)
purge.commands = find . -name '*~*' -exec rm -f {} \;

CONFIG		+= copy_dir_files qt release thread warn_on
DEFINES		+= BIBLIOTEQ_CONFIGFILE="'\"biblioteq.conf\"'" \
                   QT_DEPRECATED_WARNINGS
LANGUAGE	= C++
QT              += network sql printsupport widgets
QT              -= webkit

exists(/usr/local/include/poppler) {
DEFINES +=      BIBLIOTEQ_LINKED_WITH_POPPLER
INCLUDEPATH     += /usr/local/include/poppler/qt5
LIBS    +=      -lpoppler-qt5
}

exists(/usr/local/include/poppler/cpp) {
DEFINES +=     BIBLIOTEQ_POPPLER_VERSION_DEFINED
INCLUDEPATH += /usr/local/include/poppler/cpp
}
else {
message("The directory /usr/local/include/poppler/cpp does not exist. Poppler version information will not be available.")
}

TEMPLATE	= app

QMAKE_CLEAN	+= BiblioteQ
QMAKE_CXXFLAGS_RELEASE += -Wall \
                          -Wcast-align \
                          -Wcast-qual \
                          -Wdouble-promotion \
                          -Wextra \
                          -Wformat=2 \
			  -Wno-deprecated-declarations \
                          -Woverloaded-virtual \
                          -Wpointer-arith \
                          -Wstack-protector \
                          -Wstrict-overflow=5 \
                          -fPIE \
                          -fstack-protector-all \
                          -fwrapv \
                          -pedantic \
                          -std=c++17
QMAKE_DISTCLEAN += -r .qmake.cache .qmake.stash temp
QMAKE_EXTRA_TARGETS = purge

ICON		= Icons/book.png
INCLUDEPATH	+= Source temp

exists(/usr/local/include/yaz) {
DEFINES         += BIBLIOTEQ_LINKED_WITH_YAZ
LIBS            += -lyaz
}

PROJECTNAME	= BiblioteQ
TARGET		= BiblioteQ
