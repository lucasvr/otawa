#!/bin/bash
# $Id$

# Compute root path
if [ "$root" == "" ]; then
	root=`dirname $0`
fi

if [ "$testlist" == "" ]; then
  testlist="ct delta";
fi

# Initial configuration
tool=OBuild
version=0.6
basedir=otawa
verbose=yes
log=build.log
config=
dev=
action=make
cvs_user=anonymous
dist_flags=
systemc_location=
with_so=
done=
checked=
build_script=test.sh
making_script=
plugin_param=
testdir=deployed_tests
tags=
debug=

# functions
function display {
	if test "$verbose" = yes; then
		echo -e $*
	fi
}

function error {
	echo -e "ERROR:$*"
	exit 2
}

function say {
	if test "$verbose" = yes; then
		echo -n $* "..."
	fi
	say="$*"
}

function success {
	if test "$verbose" = yes; then
		echo "[DONE]"
	fi
	log "$say... [DONE]"
}

function failed {
	if test "$verbose" = yes; then
		echo "[FAILED]"
		if [ -n "$*" ]; then
			echo "$*"
		fi
	fi
	log "$say... [FAILED]"
	if [ -n "$*" ]; then
		log "$*"
	else
		tail $basedir/$log
	fi
	exit 1
}

function log {
	echo -e "$*\n" >> $basedir/$log
}

function log_command {
	if [ "$making_script" == yes ]; then
		echo $* '|| exit 1' >> $build_script
	else
		say "$*"
		echo "$*" | bash >> $basedir/$log 2>&1 || failed
		success
	fi
}

# get_tag module default_version
#	set VERSION variable
function get_tag {
	for tag in `echo $tags | tr "," "\n"`; do
		program=${tag%%:*}
		version=${tag#*:}
		if [ "$program" == "$1" ]; then
			VERSION="$version"
			return
		fi
	done
	VERSION="$2"
}


############# Downloads #################

function download_home {
	if [ -z "$CVS_MOD" ]; then
		CVS_MOD=$NAME
	fi
	FLAGS=
	get_tag $NAME $VERSION
	if [ -n "$VERSION" ]; then
		FLAGS="$FLAGS -r $VERSION"
	fi
	log_command cvs -d ":pserver:$cvs_user@cvs.irit.fr:/usr/local/CVS_IRIT/CVS_OTAWA" co $FLAGS $CVS_MOD
}

function download_cvs {
	log_command cvs -d $CVS_ROOT co $CVS_MOD
}

function download_wget {
	log_command wget $WGET_ADDRESS/$WGET_PACKAGE
	package=${WGET_PACKAGE%.tgz}
	if test "$package" != $WGET_PACKAGE; then
		log_command tar xvfz $WGET_PACKAGE
	else
		package=${WGET_PACKAGE%.tar.gz}
		if test "$package" != $WGET_PACKAGE; then
			log_command tar xvfz $WGET_PACKAGE
		else
			error "Unsupported archive"
		fi
	fi
	if test "$mod" != "$package"; then
		log_command ln -s $package $mod
	fi
	rm -rf $WGET_PACKAGE
}


########## update_XXX ############

function update_home {
	log_command cvs -d ":pserver:$cvs_user@cvs.irit.fr:/usr/local/CVS_IRIT/CVS_OTAWA" update
}

function update_cvs {
	log_command cvs -d $CVS_ROOT update
}

function update_wget {
	_x=
}


########## setup_XXX ############

function setup_autotool {
	if [ ! -e configure ]; then
		log_command aclocal
		log_command autoheader
		log_command automake --add-missing
		log_command autoconf
	fi
}

function setup_libtool {
	if [ ! -e configure ]; then
		log_command aclocal
		log_command autoheader
		log_command libtoolize $LIBTOOLIZE_FLAGS
		log_command automake --add-missing
		log_command autoconf
	fi
}

function setup_bootstrap {
	if [ ! -e configure ]; then
		log_command ./bootstrap
	fi
}


########## build_XXX ############

function build_autotool {
	if [ ! -e Makefile ]; then
		args="--prefix=$prefix"
		if [ "$with_so" == yes ]; then
			args="$args --enable-shared"
		fi
		if [ "$debug" == yes ]; then
			args="$args $AUTOCONF_DEBUG"
		fi
		args="$args $AUTOCONF_FLAGS"
		log_command ./configure  $args
	fi
	log_command make all $MAKE_FLAGS
}


function build_make {
	echo "make all $MAKE_FLAGS"
	echo "#!/bin/bash" > build.sh
	echo "make all $MAKE_FLAGS" >> build.sh
	log_command make all "$MAKE_FLAGS"
}


########### install_XXX ##########

function install_make {
	log_command make install
}

function install_autotool {
	install_make
}


########### distclean_XXX ##########

function distclean_autotool {
	log_command make distclean
}

function distclean_make {
	log_command make distclean
}

function distclean_clean {
	log_command make clean
}


########### Module macros ##########

function mod_elm {
	NAME=elm
	DOWNLOAD=home
	SETUP=bootstrap
	LIBTOOLIZE_FLAGS=--ltdl
	BUILD=autotool
	INSTALL=autotool
	CLEAN=autotool
	CHECK="automake-1.7 autoconf-2.59 libtool-1.5.12"
	MAKE_FLAGS="-j"
	AUTOCONF_FLAGS=""
	AUTOCONF_DEBUG="--with-mode=dev"
}

function mod_gel {
	NAME=gel
	DOWNLOAD=home
	SETUP=bootstrap
	LIBTOOLIZE_FLAGS=--ltdl
	BUILD=autotool
	INSTALL=autotool
	DISTCLEAN=autotool
	MAKE_FLAGS="-j"
	CHECK="automake-1.7 autoconf-2.59 libtool-1.5.12"
	AUTOCONF_FLAGS=""
}

function mod_frontc {
	NAME=frontc
	DOWNLOAD=wget
	WGET_ADDRESS=http://www.irit.fr/recherches/ARCHI/MARCH/frontc/
	WGET_PACKAGE=Frontc-3.2.tgz
	BUILD=make
	INSTALL=make
	DISTCLEAN=make
}

function mod_gliss {
	NAME=gliss
	DOWNLOAD=home
	BUILD=make
	DISTCLEAN=make
	VERSION=BEFORE_1_2
}

function mod_ppc {
	NAME=ppc
	DOWNLOAD=home
	BUILD=make
	MAKE_FLAGS='OPT=-DISS_DISASM GEP_OPTS="-a user0 int8 -a category int8"'
	REQUIRES="gliss gel"
	DISTCLEAN=make
}

function mod_lp_solve {
	NAME=lp_solve
	DOWNLOAD=wget
	WGET_ADDRESS=ftp://ftp.es.ele.tue.nl/pub/lp_solve/old_versions_which_you_probably_dont_want/
	WGET_PACKAGE=lp_solve_4.0.tar.gz
	BUILD=make
	MAKE_FLAGS=Makefile.linux
	DISTCLEAN=clean
}

function mod_otawa {
	NAME=otawa
	DOWNLOAD=home
	SETUP=bootstrap
	BUILD=autotool
	INSTALL=make
	REQUIRES="elm"
	DISTCLEAN=autotool
	MAKE_FLAGS="-j"
	AUTOCONF_FLAGS="$plugin_param"
	AUTOCONF_DEBUG="--with-mode=debug"
}


########### Useful functions ########

function check_program {
	program=${1%-*}
	version=${1#*-}
	expr "$checked" : "$program" > /dev/null && return

	#echo "$program -> $version"
	say "Checking $program for version $version..."
	which $program > /dev/null || failed "not found";
	cversion=`$program --version | head -1 | cut -f 4 -d " "`
	major=${version%%.*}
	cmajor=${cversion%%.*}
	minor=${version#*.}
	cminor=${cversion#*.}
	if [ "$major" \> "$cmajor" -o "${minor%%.*}" \> "${cminor%%.*}"  ]; then
		failed "at least version $version required !"
	else
		checked="$checked $program"
		success
	fi
}


########### Scan arguments ###########

function help {
	echo "$tool $version"
	echo "SYNTAX: build.sh [options] modules..."
	echo "	--with-so: use shared object libraries (enable plugins)."
	echo "	--download: just download modules."
	echo "	--make: download and make modules."
	echo "	--dev: download and make modules for internal development."
	echo "	--install: download, make and install modules."
	echo "	--dist: download and generate a distribution."
	echo "	-h|--help: display this message."
	echo "	--prefix=PATH: target path of the build."
	echo "	--build=PATH: directory to build in."
	echo "	--release=NUMBER: release of the distribution."
	echo "	--proxy=ADDRESS:PORT: configure a proxy use."
	echo "	--update: update the current installation."
	echo "	--with-systemc: SystemC location."
	echo "	--check: download, make, install, and test."
	echo "	--checkonly: test only."
	echo "  --tag=module:version: use the given CVS version for the module."
	echo "  --debug: use debug options to build the modules."
	echo "MODULES: elm gliss ppc lp_solve frontc otawa"
}

modules=
for arg in $*; do
	case $arg in
	--prefix=*)
		prefix=${arg#--prefix=}
		;;
	--build=*)
		basedir=${arg#--build=}
		;;
	--make)
		action=make
		;;
	--dev)
		action=dev
		cvs_user=$LOGNAME
		;;
	--download)
		action=download
		;;
	--check)
		action=check
		;;
	--checkonly)
		action=checkonly
		;;
	--dist)
		action=dist
		;;
	--release=*)
		action=dist
		release=${arg#--release}
		dist_flags="$dist_flags RELEASE=$release"
		;;
	--with-systemc=*)
		systemc_location=${arg#--with-systemc=}
		;;
	--install)
		action=install
		;;
	--update)
		action=update
		;;
	--with-so)
		with_so=yes
		plugin_param="--with-plugin=yes"
		;;
	--proxy=*)
		export http_proxy=${arg#--proxy=}
		export ftp_proxy=${arg#--proxy=}
		;;
	--tag=*)
		tags="$tags,${arg#--tag=}"
		;;
	--debug|-d)
		debug=yes
		;;
	-h|--help)
		help
		exit
		;;
	-*|--*)
		help
		error "unknown option \"$arg\"."
		;;
	*)
		modules="$modules $arg"
		;;
	esac
done
if [ -z "$modules" ]; then
	modules="gliss gel ppc lp_solve frontc elm otawa"
fi
if [ $action = update ]; then
	updates="$modules"
fi
if [ -z "$prefix" ]; then
	prefix=$basedir
fi
if [ "${prefix:0:1}" != "/" ]; then
	prefix="$PWD/$prefix"
fi


# Make the build basedirectory
mkdir -p $basedir
cd $basedir
if [ ! -z $systemc_location ]; then
	rm -f systemc
	ln -s $systemc_location systemc
fi

basedir=`pwd`

# Preparation
echo "Build by $tool $version (" `date` ")\n" > $log
if test -n "$config"; then
	if test -f $config; then
		. $config
	else
		error "cannot use the configuration file $config."
	fi
fi


########### Process a module ###########
function process {
	expr "$done" : "$1" > /dev/null && return
	
	# Perfom requirements first
	REQUIRES=
	mod_$1
	for m in $REQUIRES; do
		if [ $action != update -o `expr match "$updates" ".*$m.*"` != 0 ]; then
			process $m
		else
			action=make
			process $m
			action=update
		fi
	done

	# Configure
	BUILD=
	INSTALL=
	DOWNLOAD=
	PATCH=
	MAKE_FLAGS=
	CVS_MOD=
	WGET_ADDRESS=
	WGET_PACKAGE=
	SETUP=
	LIBTOOLIZE_FLAGS=
	CHECK=
	VERSION=
	DISTCLEAN=
	mod_$1
	echo
	echo "*** Module $1 [$done] $action ***"
	done="$done $1"

	# Perform checking
	for check in $CHECK; do
		check_program $check
	done
	
	# Perform the download
	if [ -n "$DOWNLOAD" ]; then
		if [ ! -d $1 ]; then
			echo "Download parce que $1 n'existe pas"
			download_$DOWNLOAD
		else
			if [ $action = update ]; then
				cd $1
				if [ -n "$DISTCLEAN" ]; then
					distclean_$DISTCLEAN
				fi
				update_$DOWNLOAD
				cd $basedir
			fi
		fi
	fi
	
	# Requires setup and build ?
	case "$action" in
	update|make|install|dev|check)
		if [ -n "$SETUP" ]; then
			cd $1
			setup_$SETUP
			cd $basedir
		fi
		if [ -n "$BUILD" ]; then
			cd $1
			build_$BUILD
			cd $basedir
		fi
		;;
	esac
	
	# Perform install
	case "$action" in
	install|check)
		if [ -n "$INSTALL" ]; then
			cd $1
			install_$INSTALL
			cd $basedir
		fi
		;;
	esac
	
	
	# Perform distribution
	case "$action" in
	dist)
		old_prefix="$prefix"
		prefix='$prefix'
		making_script=yes
		echo "cd $1" >> $build_script
		if [ -n "$SETUP" ]; then
			setup_$SETUP
		fi
		if [ -n "$BUILD" ]; then
			build_$BUILD
		fi
		if [ -n "$INSTALL" ]; then
			install_$INSTALL
		fi
		echo "cd .." >> $build_script
		macking_script=
		prefix="$old_prefix"
		;;
	esac

}

function do_tst {

	say "Processing test: $1"
	if [ ! -d $basedir/$testdir/$1 ]; then
		mkdir -p $basedir/$testdir/$1
	fi
	cp $basedir/otawa/test/$1/*.cpp $basedir/otawa/test/$1/*.h $basedir/$testdir/$1/ 2>/dev/null
	objects="`ls $basedir/otawa/test/$1/*.cpp |while read A ; do basename $A |sed 's/\.cpp$/.o/g' ; done `"
	objects="`echo $objects`"
	cat <<EOF > $basedir/$testdir/$1/Makefile
CXXFLAGS=\$(shell otawa-config --cflags gensim display ppc lp_solve) -DDATA_DIR="\"\$(shell otawa-config --data gensim display ppc lp_solve)\""
LDLIBS=\$(shell otawa-config --libs gensim display ppc lp_solve)
LDFLAGS=-dlopen force
CXX=libtool --mode=compile --tag=CXX g++
CC=libtool --mode=link --tag=CXX g++
all: test_$1
test_$1: $objects
clean:
	rm -rf *.o test_$1 *~ core *.lo .libs
EOF
	success
	
	say "Building test: $1"
	
	(
		cd $basedir/$testdir/$1
		PATH=$PATH:$prefix/bin/ make 
	) || error "Test build failed."
	success
	
	say "Checking for: $1"
	cd $basedir/otawa/test/$1/
	../../bin/check.sh --benchdir $basedir/snu-rt/ $basedir/$testdir/$1/test_$1 || error "Test failed."
	success
	
}


# Process modules
if [ "$action" == dist ]; then
	echo '#!/bin/bash' > $build_script
	echo 'prefix=$PWD' >> $build_script
	echo 'if [ -n "$1" ]; then' >> $build_script
	echo '	prefix="$1"' >> $build_script
	echo 'fi' >> $build_script
	chmod +x $build_script
fi

done=
for mod in $modules; do
	process $mod
done


# Perform test
case "$action" in
checkonly|check)
	echo "Doing tests..."
	if [ ! -d $basedir/snu-rt ]; then
		echo "Downloading the benchs"
		WGET_ADDRESS="http://www.irit.fr/recherches/ARCHI/MARCH/frontc/" WGET_PACKAGE="snu-rt.tgz" mod="snu-rt" download_wget
	fi
	for tst in $testlist; do
		do_tst $tst
	done
esac


if [ "$action" == dist ]; then
	echo "Not implemented yet."
fi
