#define _GNU_SOURCE
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <math.h>

#define	INVALID_FD	(-1)
#define ARRAY_SIZE(a)	(sizeof(a) / sizeof(a)[0])

#if (!defined(PAGE_SIZE))
#define PAGE_SIZE	(4096)
#endif /* (!defined(PAGE_SIZE)) */

#define	__force

typedef	struct {
	char		*Path;		/*!< port path.	*/
	int		Fd;		/*!< port file descriptor. */
	struct termios	TermiosSave;	/*!< saved termio configuration. */
	struct termios	TermiosCur;	/*!< current termio configuration. */
	struct timespec	CharWait;	/*!< Wait Time per character. */
} PortM202MD10B;

typedef enum {
	AS_OPT_AND_STR,
	AS_STR_ONLY,
} ArgStates;

typedef struct {
	int		Argc0;
	char		**Argv0;
	int		Argc;
	char		**Argv;
	int		ArgcNextOpt;
	char		**ArgvNextOpt;
	const char	*OptShort;
	const char	*OptLong;
	ssize_t		OptLength;
	const char	*OptParam;
	int		OptParamEqual;
	int		OptParamCont;
	int		OptDash;
	int		OptDashDash;
	int		ParseOnly;
	ArgStates	AState;
	const char	*Path;
	long		Baud;
	speed_t		stBaud;
	int		RtsCts;
	int		CWaitMs;
	PortM202MD10B	Port;
} CommandLine;

CommandLine	CLine;

#define	M202MD10B_BAUD_MIN	(1200)
#define	M202MD10B_MAUD_MAX	(9600)
#define	M202MD10B_BAUD_DEF	(2400)
#define	M202MD10B_BAUD_DEF_ST	(B2400)

typedef	struct {
	long	Baud;
	speed_t	StVal;
} BaudSppedTMap;

BaudSppedTMap BaudSppedTTable[] = {
	{      0,       B0},
	{     50,      B50},
	{     75,      B75},
	{    110,     B110},
	{    134,     B134},
	{    150,     B150},
	{    200,     B200},
	{    300,     B300},
	{    600,     B600},
	{   1200,    B1200},
	{   1800,    B1800},
	{   2400,    B2400},
	{   4800,    B4800},
	{   9600,    B9600},
	{  19200,   B19200},
	{  38400,   B38400},
	{  57600,   B57600},
	{ 115200,  B115200},
	{ 230400,  B230400},
	{ 460800,  B460800},
	{ 500000,  B500000},
	{ 576000,  B576000},
	{ 921600,  B921600},
	{1000000, B1000000},
	{1152000, B1152000},
	{1500000, B1500000},
	{2000000, B2000000},
	{2500000, B2500000},
	{3000000, B3000000},
	/* Some OS and Seral port may have more higer speed. */
	{     -1,       B0},
};

speed_t BaudToSpeedT(long baud, int *valid)
{	int		dummy;
	BaudSppedTMap	*table;

	if (!valid) {
		valid = &dummy;
	}

	*valid = 0;
	table = BaudSppedTTable;
	while (table->Baud >= 0) {
		if (table->Baud == baud) {
			*valid = 1;
			return table->StVal;
		}
		table++;
	}
	return B0;
}

int SpeedTIsM202(speed_t stVal)
{	switch (stVal) {
	case B1200:
	case B2400:
	case B4800:
	case B9600:
		return 1;
	default:
		break;
	}
	return 0;
}

/*! Init serial port context.
 */
int PortM202MD10BInit(PortM202MD10B *p)
{	memset(p, 0, sizeof(*p));
	p->Fd = INVALID_FD;
	return 1;
}

/*! Close serial port.
 */
int PortM202MD10BClose(PortM202MD10B *p)
{	int	ret = 1;

	if (p->Fd != INVALID_FD) {
		/* opened. */
		/* Restore Terminal configuration. */
		if (tcsetattr(p->Fd, TCSANOW, &(p->TermiosSave)) != 0) {
			fprintf(stderr,
				"%s: Can not restore configuration. %s(%d).\n",
				p->Path,
				strerror(errno),
				errno
			);
			ret = 0;
		}
		if (close(p->Fd) != 0) {
			fprintf(stderr,
				"%s: Can not close. %s(%d).\n",
				p->Path,
				strerror(errno),
				errno
			);
			ret = 0;
		}
		p->Fd = INVALID_FD;
	}
	if (p->Path){
		/* Allocated path. */
		free(p->Path);
		p->Path = NULL;
	}
 return ret;
}

/*! open serial port.
*/
int PortM202MD10BOpen(PortM202MD10B *p, const char *Path)
{	int	fd;

	if (p->Fd != INVALID_FD) {
		PortM202MD10BClose(p);
	}

	p->Path = strdup(Path);
	if (!(p->Path)) {
		fprintf(stderr,"%s.%s: Can not allocate memory. %s(%d).\n",
			*(CLine.Argv0),
			__func__,
			strerror(errno),
			errno
		);
		return 0;
	}

	fd = open(Path, O_RDONLY,0);
	if (fd < 0) {
		fprintf(stderr,"%s: Can not open.%s(%d).\n",
			p->Path,
			strerror(errno),
			errno
		);
		goto out_err_open;
	}
	p->Fd = fd;
	if (tcgetattr(p->Fd, &(p->TermiosSave)) != 0) {
		 fprintf(stderr,
			"%s: Can not get terminal configuration. %s(%d).\n",
			p->Path,
			strerror(errno),
			errno
		);
		goto out_err_getattr;
	}
	memcpy(&(p->TermiosCur),
		&(p->TermiosSave),
		sizeof(p->TermiosCur)
	);
	return 1;

out_err_getattr:
	close(p->Fd);
out_err_open:
	free(p->Path);
	p->Path = NULL;
	return 0;
}

/*! Configure serial port.
*/
int PortM202MD10BConfig(PortM202MD10B *p, speed_t stVal, int rtscts, long ch_wait_ms)
{	int	ret;
	ldiv_t	qr;

	ret = 1;
	cfmakeraw(&(p->TermiosCur));
	if (cfsetispeed(&(p->TermiosCur), stVal) != 0) {
		fprintf(stderr,
			"%s: Can not set input speed. %s(%d).\n",
			p->Path,
			strerror(errno),
			errno
		);
		return 0;
	}

	if (cfsetospeed(&(p->TermiosCur), stVal) != 0) {
		fprintf(stderr,
			"%s: Can not set output speed. %s(%d).\n",
			p->Path,
			strerror(errno),
			errno
		);
		return 0;
	}

	p->TermiosCur.c_iflag &= ~(IXON | IXOFF);

	/* 8bit, NoParity, 1stop, */
	p->TermiosCur.c_cflag &= ~(CSIZE | CSTOPB | PARENB);
	p->TermiosCur.c_cflag |= CS8 | CLOCAL;

	if (rtscts) {
		p->TermiosCur.c_cflag |=  CRTSCTS;
	} else {
		p->TermiosCur.c_cflag &= ~CRTSCTS;
	}

	/* disable output processing */
	p->TermiosCur.c_oflag &= ~(OPOST | OLCUC | ONLCR | OCRNL | ONOCR | OFILL);


	if (tcsetattr(p->Fd, TCSANOW, &(p->TermiosCur)) != 0) {
		fprintf(stderr,
		"%s: Can not set configuration. %s(%d).\n",
			p->Path,
			strerror(errno),
			errno
		);
		ret = 0;
	}

	if (ch_wait_ms < 0) {
		ch_wait_ms = 0;
	}

	qr = ldiv(ch_wait_ms, 1000);
	p->CharWait.tv_sec =  qr.quot;
	p->CharWait.tv_nsec = qr.rem * 1000 * 1000;
	return ret;
}

int PortM202MD10BWrite(PortM202MD10B *p, const uint8_t *buf, ssize_t len)
{	ssize_t		wlen = 0;
	ssize_t		ret;
	ssize_t		result = 1;
#define	ZERO_CNTR_MAX	(256)
	int		zero_cntr = 0;

	if ((p->CharWait.tv_sec == 0) && (p->CharWait.tv_nsec == 0)) {
		while (wlen < len) {
			ret = write(p->Fd, buf, len);
			if (ret < 0) {
				fprintf(stderr, "%s: Can not write, %s(%d). len=%ld\n",
					p->Path, strerror(errno), errno, (long)(len)
				);
				result = 0;
				break;
			} else {
				if (ret == 0) {
					zero_cntr++;
					if (zero_cntr >= ZERO_CNTR_MAX) {
						fprintf(stderr,
							"%s: Too many zero length written.\n",
							p->Path
						);
						result = 0;
						break;
					}
				} else {
					zero_cntr = 0;
				}
			}
			wlen += ret;
			buf +=  ret;
		}
	} else {
		while (wlen < len) {
			ret = write(p->Fd, buf, 1);
			if (ret < 0) {
				fprintf(stderr, "%s: Can not write, %s(%d).\n",
					p->Path, strerror(errno), errno
				);
				result = 0;
				break;
			} else {
				if (ret == 0) {
					zero_cntr++;
					if (zero_cntr >= ZERO_CNTR_MAX) {
						fprintf(stderr,
							"%s: Too many zero length written.\n",
							p->Path
						);
						result = 0;
						break;
					}
				} else {
					zero_cntr = 0;
				}
			}
			if (nanosleep(&(p->CharWait), NULL)) {
				fprintf(stderr, "%s: nanosleep(): %s(%d).\n",
					*(CLine.Argv0),
					strerror(errno), errno
				);
				result = 0;
				break;
			}
			wlen++;
			buf++;
		}
	}
	return result;
}

int PortM202MD10Break(PortM202MD10B *p)
{	int	result = 1;

	if (tcsendbreak(p->Fd, 0)) {
		fprintf(stderr, "%s: Can not send break, %s(%d).\n",
			p->Path, strerror(errno), errno
		);
		result = 0;
	}
	return result;
}

const char *StrSkipToChar(const char *p, char c)
{	char	t;

	if (!p) {
		return p;
	}

	while ((t = *p) != '\0') {
		if (t == c) {
			return p;
		}
		p++;
	}
	return p;
}

int CommandLineInit(CommandLine *cl, int argc, char **argv)
{	cl->Argc0 = argc;
	cl->Argv0 = argv;
	argc--;
	argv++;
	cl->Argc = argc;
	cl->Argv = argv;
	cl->ArgcNextOpt = 0;
	cl->ArgvNextOpt = NULL;
	cl->OptShort = NULL;
	cl->OptLong = NULL;
	cl->OptLength = 0;
	cl->OptParam = NULL;
	cl->OptParamEqual = 0;
	cl->OptParamCont = 0;
	cl->OptDash = 0;
	cl->OptDashDash = 0;
	cl->ParseOnly = 0;
	cl->AState = AS_OPT_AND_STR;
	cl->Path = NULL;
	cl->Baud =   M202MD10B_BAUD_DEF;
	cl->stBaud = M202MD10B_BAUD_DEF_ST;
	cl->RtsCts = 0;
	cl->CWaitMs = 0;
	if (!PortM202MD10BInit(&(cl->Port))) {
		return 0;
	}
	return 1;
}

void CommandLineDone(CommandLine *cl)
{	PortM202MD10BClose(&(cl->Port));
}

int CommandLineReset(CommandLine *cl)
{	cl->Argc = cl->Argc0 + 1;
	cl->Argv = cl->Argv0 + 1;
	return 1;
}

int CommandLineEoa(CommandLine *cl)
{	char	**argv;

	if (cl->Argc <= 0) {
		return 1;
	}
	argv = cl->Argv;
	if (*argv == NULL) {
		return 1;
	}
	return 0;
}

int CommandLineLongOptPrepare(CommandLine *cl)
{	const char	*p;
	const char	*p2;
	char		**argv;

	cl->OptShort = NULL;
	cl->OptLong = NULL;
	cl->OptLength = 0;
	cl->OptParam = NULL;
	cl->OptParamEqual = 0;
	cl->OptParamCont = 0;
	cl->OptDash = 0;
	cl->OptDashDash = 0;

	if (cl->Argc <= 0) {
		return 0;
	}

	p = *(cl->Argv);
	if (p == NULL) {
		return 0;
	}
	if (*p == '\0') {
		/* Zero Length Argument. */
		return 1;
	}

	if (*p == '-') {
		p++;
		if (*p != '-') {
			/* Short Option */
			cl->OptShort = p;
			if (*p) {
				p++;
				if (*p) {
					/* Option and Parameter in one argument. */
					if (*p != '=') {
						cl->OptParam = p;
						cl->OptParamCont = 1;
						cl->ArgcNextOpt = cl->Argc - 1;
						cl->ArgvNextOpt = cl->Argv + 1;
					} else {
						p++;
						cl->OptParam = p;
						cl->OptParamCont = 1;
						cl->ArgcNextOpt = cl->Argc - 1;
						cl->ArgvNextOpt = cl->Argv + 1;
					}
				} else {
					/* Option in one argument. */
					argv = cl->Argv;
					argv++;
					p2 = *argv;
					if ((p2) && (cl->Argc > 1)) {
						/* There is next argument. */
						cl->OptParam = p2;
						cl->ArgcNextOpt = cl->Argc - 2;
						cl->ArgvNextOpt = cl->Argv + 2;
					} else {
						/* No next argument. */
						cl->ArgcNextOpt = cl->Argc - 1;
						cl->ArgvNextOpt = cl->Argv + 1;
					}
				}
			} else {
				/* Single dash only. */
				cl->OptDash = 1;
				cl->ArgcNextOpt = cl->Argc - 1;
				cl->ArgvNextOpt = cl->Argv + 1;
			}
		} else {
			/* Long Option */
			ssize_t		l;
			p++;
			cl->OptLong = p;
			p2 = StrSkipToChar(p, '=');
			l = p2 - p;
			cl->OptLength = l;
			if (l > 0) {
				if (*p2 == '=') {
					/* Option parameter by equal .*/
					p2++;
					cl->OptParam = p2;
					cl->OptParamEqual = 1;
					cl->OptParamCont = 1;
					cl->ArgcNextOpt = cl->Argc - 1;
					cl->ArgvNextOpt = cl->Argv + 1;
				} else {
					/* Next argument may be option parameter */
					argv = cl->Argv;
					argv++;
					p2 = *argv;
					if ((p2) && (cl->Argc > 1)) {
						/* There is next argument. */
						cl->OptParam = p2;
						cl->ArgcNextOpt = cl->Argc - 2;
						cl->ArgvNextOpt = cl->Argv + 2;
					} else {
						/* No next argument. */
						cl->ArgcNextOpt = cl->Argc - 1;
						cl->ArgvNextOpt = cl->Argv + 1;
					}
				}
			}  else {
				/* double dash only. */
				cl->OptDashDash = 1;
				cl->ArgcNextOpt = cl->Argc - 1;
				cl->ArgvNextOpt = cl->Argv + 1;
			}
		}
	}
	return 1;
}

typedef enum {
	M202MD10B_HARD_RESET = 0xFF,	/*!< Pseudo code */
	M202MD10B_BRIGHT_4 =			0x01,
	M202MD10B_BRIGHT_3 =			0x02,
	M202MD10B_BRIGHT_2 =			0x03,
	M202MD10B_BRIGHT_1 =			0x04,
	M202MD10B_BACK =			0x08,
	M202MD10B_FORWARD =			0x09,
	M202MD10B_CLEAR_ALL =			0x0A,
	M202MD10B_CLEAR_ALL_GOTO_HOME =		0x0C,
	M202MD10B_GOTO_HOME =			0x0D,
	M202MD10B_ROUND_TRIP_MODE =		0x11,
	M202MD10B_SCROLL_MODE =			0x12,
	M202MD10B_RESET_BLINK_SHOW_CURSOR =	0x13,
	M202MD10B_HIDE_CURSOR =			0x14,
	M202MD10B_BLINK_CURSOR =		0x15,
	M202MD10B_UNDER_LINE_CURSOR =		0x16,
	M202MD10B_BLOCK_CURSOR =		0x17,
	M202MD10B_REVERSE_CURSOR =		0x18,
	M202MD10B_DEFINE_CHAR =			0x1A,
	M202MD10B_GOTO_POSITION =		0x1B,
	M202MD10B_CWAITMS = 0xF9,	/*!< Pseudo code */
	M202MD10B_RTSCTS = 0xFA,	/*!< Pseudo code */
	M202MD10B_BAUD = 0xFB,		/*!< Pseudo code */
	M202MD10B_HELP = 0xFC,		/*!< Pseudo code */
	M202MD10B_PATH = 0xFD,		/*!< Pseudo code */
	M202MD10B_NOT_MATCH = 0xFE,	/*!< Pseudo code */
} M202MD10BControlCodes;

#define M202MD10B_ADDRESS_MAX	(39)
#define M202MD10B_ADDRESS_WIDTH	(40)
#define M202MD10B_WIDTH		(20)
#define M202MD10B_HEIGHT	 (2)


typedef int CommandLineOptionFn(CommandLine *cl, unsigned char send);

typedef struct {
	ssize_t			Length;
	const char		*Option;
	unsigned char		Send;
	CommandLineOptionFn	*Fn;
} LongMatch;

int CommandLineHardReset(CommandLine *cl, unsigned char send)
{	int	result = 1;

	cl->Argc--;
	cl->Argv++;
	if (cl->ParseOnly) {
		return result;
	}
	result = PortM202MD10Break(&(cl->Port));
	return result;
}

int CommandLineBright(CommandLine *cl, unsigned char send)
{	int		result = 1;
	long		val;
	const char	*opt;
	char		one_char;
	char		*p2;

	opt = cl->OptShort;
	one_char = (opt ? *opt : '\0');
	if (one_char) {
		/* Short Option */
		if ((one_char >='a') && (one_char <='d')) {
			send = M202MD10B_BRIGHT_4 + (one_char - 'a');
			cl->Argc--;
			cl->Argv++;
		} else {
			fprintf(stderr, "%s: %s: Invalid bright level, may be bug.\n",
				*(cl->Argv0),
				*(cl->Argv)
			);
			cl->Argc--;
			cl->Argv++;
			return 0;
		}
	} else {
		/* Long Option */
		opt = cl->OptParam;
		if (!opt) {
			fprintf(stderr, "%s: %s: Need bright level parameter by 1 to 4.\n",
				*(cl->Argv0),
				*(cl->Argv)
			);
			cl->Argc--;
			cl->Argv++;
			return 0;
		}

		p2 = (__force char *)opt;
		val = strtol(opt, &p2, 0);
		if (opt == p2) {
			fprintf(stderr, "%s: %s: Specify bright level by 1 to 4.\n",
				*(cl->Argv0),
				*(cl->Argv)
			);
			cl->Argc = cl->ArgcNextOpt;
			cl->Argv = cl->ArgvNextOpt;
			return 0;
		}

		if ((val < 1) || (val > 4)) {
			fprintf(stderr, "%s: %s: Out of range bright level, specify 1 to 4.\n",
				*(CLine.Argv0),
				*(cl->Argv)
			);
			send = M202MD10B_BRIGHT_4 + (4 - val);
			cl->Argc = cl->ArgcNextOpt;
			cl->Argv = cl->ArgvNextOpt;
		}
	}

	if (cl->ParseOnly) {
		return result;
	}
	result = PortM202MD10BWrite(&(cl->Port), &send, sizeof(send));
	return result;
}

int CommandLineBackForward(CommandLine *cl, unsigned char send_p, unsigned char send_n, const char *msg_word)
{	int		result = 1;
	long		val = 1;
	const char	*opt;
	unsigned char	send = send_p;
	char		*p2;

	opt = cl->OptShort;
	if ((opt) && (*opt)) {
		/* Short option. */
		if (cl->OptParamCont) {
			opt = cl->OptParam;
			if (*opt) {
				p2 = (__force char *)opt;
				val = strtol(opt, &p2, 0);
				if (opt == p2) {
					fprintf(stderr, "%s: %s: Append %s character(s) in number.\n",
						*(cl->Argv0),
						*(cl->Argv),
						msg_word
					);
					cl->Argc--;
					cl->Argv++;
					return 0;
				}
			}
		} else {
			val = 1;
		}
		cl->Argc--;
		cl->Argv++;
	} else {
		/* Long option. */
		if (cl->OptParamCont) {
			opt = cl->OptParam;
			if (*opt) {
				p2 = (__force char *)opt;
				val = strtol(opt, &p2, 0);
				if (opt == p2) {
					fprintf(stderr, "%s: %s: Append %s character(s) in number.\n",
						*(cl->Argv0),
						*(cl->Argv),
						msg_word
					);
					cl->Argc--;
					cl->Argv++;
					return 0;
				}
			}
		} else {
			val = 1;
		}
		cl->Argc--;
		cl->Argv++;
	}

	if (cl->ParseOnly) {
		return result;
	}

	if (val < 0) {
		val = -val;
		send = send_n;
	}
	while ((result) && (val > 0)) {
		result = PortM202MD10BWrite(&(cl->Port), &send, sizeof(send));
		val--;
	}
	return result;
}

int CommandLineBack(CommandLine *cl, unsigned char send)
{	return CommandLineBackForward(cl, M202MD10B_BACK, M202MD10B_FORWARD, "back");
}

int CommandLineForward(CommandLine *cl, unsigned char send)
{	return CommandLineBackForward(cl, M202MD10B_FORWARD, M202MD10B_BACK, "forward");
}

int CommandLineOneChar(CommandLine *cl, unsigned char send)
{	int		result = 1;

	/* Simply take next argument. */
	cl->Argc--;
	cl->Argv++;
	if (cl->ParseOnly) {
		return result;
	}

	result = PortM202MD10BWrite(&(cl->Port), (const uint8_t *)&send, sizeof(send));
	return result;
}

unsigned long DigitChar(char c)
{	char		buf[2];
	char		*p2;
	unsigned long	val;

	buf[0] = c;
	buf[1] = '\0';
	p2 = buf;
	val = strtoul(buf, &p2, 36);
	if (buf == p2) {
		return 0xff;
	}
	return val;
}

typedef	struct {
	char	Code;
	char	Def;
	char	Lines[7];
} M202MD10BDefineChar;

int CommandLineDefineChar(CommandLine *cl, unsigned char send_cmd)
{	unsigned char	send[sizeof(M202MD10BDefineChar)];
	const char	*p;
	char		c;
	unsigned long	d;
	unsigned char	a;
	ssize_t		i;
	ssize_t		seq;
	int		result = 1;

	memset(send, 0, sizeof(send));
	i = 0;
	send[i] = send_cmd;
	i++;
	p = cl->OptParam;
	a = 0;
	seq = 0;
	while ((i < sizeof(send)) && (p) && ((c = *p) != 0)) {
		d = DigitChar(c);
		if (d > 0xf) {
			send[i] = a;
			i++;
			a = 0;
			seq = 0;
		} else {
			a <<= 0x4;
			a += (unsigned char)d;
			seq++;
			if (seq >= 2) {
				send[i] = a;
				i++;
				a = 0;
				seq = 0;
			}
		}
		p++;
	}
	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	if (cl->ParseOnly) {
		return result;
	}
	result = PortM202MD10BWrite(&(cl->Port), send, sizeof(send));
	return result;
}

int CommandLineGotoPosition(CommandLine *cl, unsigned char send_cmd)
{	unsigned char	send[2];
#define	GOTO_POSITION_LOCATIONS		(2)
	long		pos[GOTO_POSITION_LOCATIONS];
	const char	*p;
	char		*p2;
	long		a;
	long		x;
	long		y;
	ssize_t		seq;
	int		result = 1;

	send[0] = send_cmd;

	memset(pos, 0, sizeof(pos));
	seq = 0;
	p = cl->OptParam;
	while ((seq < ARRAY_SIZE(pos)) && (p) && (*p != '\x0')) {
		p2 = (__force char *)p;
		a = strtol(p, &p2, 0);
		if (p == p2) {
			fprintf(stderr, "%s: %s: Specify position in number. parameter=%s\n",
				*(cl->Argv0),
				*(cl->Argv), cl->OptParam
			);
			cl->Argc = cl->ArgcNextOpt;
			cl->Argv = cl->ArgvNextOpt;
			return 0;
		}
		pos[seq] = a;
		seq++;
		p = p2;
		switch (*p) {
		case ',':
		case ':':
		case ' ':
			p++;
			break;
		case '\0':
			break;
		default:
			fprintf(stderr, "%s: %s: Specify position by address or x,y. parameter=%s\n",
				*(cl->Argv0),
				*(cl->Argv), cl->OptParam
			);
			cl->Argc = cl->ArgcNextOpt;
			cl->Argv = cl->ArgvNextOpt;
			return 0;
		}
	}
	switch (seq) {
	case 0:
		fprintf(stderr, "%s: %s: No position parameter. parameter=%s\n",
			*(CLine.Argv0),
			*(cl->Argv), cl->OptParam
		);
		cl->Argc = cl->ArgcNextOpt;
		cl->Argv = cl->ArgvNextOpt;
		return 0;
	case 1:
		/* Position by address. */
		a = pos[0];
		a %= (M202MD10B_ADDRESS_MAX + 1);
		if (a < 0) {
			a = (M202MD10B_ADDRESS_MAX + 1) + a;
		}
		send[1] = (unsigned char)(a);
		break;
	default:
		/* Position by x,y */
		x = pos[0];
		x %= M202MD10B_WIDTH;
		if (x < 0) {
			x = M202MD10B_WIDTH + x;
		}
		y = pos[1];
		y %= M202MD10B_HEIGHT;
		if (y < 0) {
			y = M202MD10B_HEIGHT + y;
		}
		send[1] = (unsigned char)(x + y * M202MD10B_HEIGHT);
		break;
	}
	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	if (cl->ParseOnly) {
		return result;
	}
	result = PortM202MD10BWrite(&(cl->Port), send, sizeof(send));
	return result;
}

int CommandLineSendArg(CommandLine *cl, unsigned char send_cmd)
{	const uint8_t		*send;
	ssize_t			len;
	int			result = 1;

	send = (__force const uint8_t *)*(cl->Argv);
	cl->Argc--;
	cl->Argv++;
	if (cl->ParseOnly) {
		return result;
	}
	len = strlen((__force const char *)send);
	result = PortM202MD10BWrite(&(cl->Port), send, len);
	return result;
}

int CommandLineSendStdIn(CommandLine *cl, unsigned char send_cmd)
{	unsigned char	*send;
	ssize_t		len;
	long		sc_page_size;
	int		result = 1;
	int		fd;

	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	if (cl->ParseOnly) {
		return result;
	}
	sc_page_size = sysconf(_SC_PAGESIZE);
	if (sc_page_size < 0) {
		sc_page_size = PAGE_SIZE;
	}

	send = malloc(sc_page_size);
	if (!send) {
		fprintf(stderr, "%s: Can not allocate read buffer, %s(%d). sc_page_size=%ld\n",
			*(CLine.Argv0),
			strerror(errno), errno,
			(long)(sc_page_size)
		);
		return 0;
	}

	fd = fileno(stdin);
	while ((result != 0) && ((len = read(fd, send, sc_page_size)) >= 0)) {
		result = PortM202MD10BWrite(&(cl->Port), send, sizeof(len));
	}
	if (len < 0) {
		fprintf(stderr, "%s: Can not read stdin, %s(%d).\n",
			*(CLine.Argv0),
			strerror(errno), errno
		);
		result = 0;
	}
	free(send);
	return result;
}

int CommandLinePath(CommandLine *cl, unsigned char send_cmd)
{
	cl->Path = cl->OptParam;
	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	return -1;
}

int CommandLineBaud(CommandLine *cl, unsigned char send_cmd)
{	long		b;
	const char	*p;
	char		*p2;
	int		result = 1;

	p = cl->OptParam;
	p2 = (__force char *)p;
	b = strtol(p, &p2, 0);
	if (p == p2) {
		cl->Argc = cl->ArgcNextOpt;
		cl->Argv = cl->ArgvNextOpt;
		return 0;
	}
	cl->Baud = b;
	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	return result;
}

int CommandLineCWaitMs(CommandLine *cl, unsigned char send_cmd)
{	long		w;
	const char	*p;
	char		*p2;
	int		result = 1;

	p = cl->OptParam;
	p2 = (__force char *)p;
	w = strtol(p, &p2, 0);
	if (p == p2) {
		cl->Argc = cl->ArgcNextOpt;
		cl->Argv = cl->ArgvNextOpt;
		return 0;
	}
	if (w < 0) {
		w = 0;
	}
	cl->CWaitMs = w;
	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	return result;
}

int CommandLineRtsCts(CommandLine *cl, unsigned char send_cmd)
{	long		h;
	char		c;
	const char	*p;
	char		*p2;
	int		result = 1;

	p = cl->OptParam;
	c = *p;
	switch (c) {
	case 'y':
	case 'Y':
	case '+':
		h = 1;
		break;
	case 'n':
	case 'N':
	case '-':
		h = 0;
		break;
	default:
		p2 = (__force char *)p;
		h = strtol(p, &p2, 0);
		if (p == p2) {
			cl->Argc = cl->ArgcNextOpt;
			cl->Argv = cl->ArgvNextOpt;
			return 0;
		}
	}

	cl->RtsCts = h;
	cl->Argc = cl->ArgcNextOpt;
	cl->Argv = cl->ArgvNextOpt;
	return result;
}

int CommandLineHelp(CommandLine *cl, unsigned char send_cmd)
{	cl->Argc--;
	cl->Argv++;
	fprintf(stdout,
		"m202md10b [-R] [-a] [-b] [-c] [-d] [-h[N]] [-i[N]] [-j] [-l] [-m]\n"
		"[-q] [-r] [-s] [-t] [-u] [-v] [-w] [-x]\n"
		"[-z c:l0:l1:l2:l3:l4:l5:l6] [-[ [i|x,y]]\n"
		"[-B baud] [-C [y|n]] [-W m] [-F path]  [-H]\n"
		"-R     Hard Reset.               -q   Round Trip Mode.\n"
		"-a     Bright 4 (Max).           -r   Scroll Mode.\n"
		"-b     Bright 3.                 -s   Reset Blink and Show Cursor.\n"
		"-c     Bright 2.                 -t   Hide Cursor.\n"
		"-d     Bright 1 (Min),           -u   Blink Cursor.\n"
		"-h[N]  Back cursor N step(s).    -v   Under Line Cursor.\n"
		"-i[N]  Forward cursor N step(s). -w   Block Cursor.\n"
		"-j     Clear All.                -x   Reverse Cursor.\n"
		"-l     Clear All and Goto Home.\n"
		"-m     Goto Home.\n"
		"-z c:L0:L1:L2:L3:L4:L5:L6 Define Character,\n"
		"   c, and L0..L6 are based on hex.\n"
		"-[ i|x,y  Goto Position i or (x, y).\n"
		"-B baud   Set baud rate, baud = {1200, 2400(*), 4800, 9600}..\n"
		"-C y|n    RTS-CTS mode, y: Enable, n: Disable(*).\n"
		"-W m      Wait m milli second(s) per character.\n"
		"-F path   Serial port device path.\n"
		"-H        Help message.\n"
		"          (*) default."
	);
	return 0;
}

#define	LMID_STR(s)	(sizeof(s) - 1), s

LongMatch LongMatchTable[] = {
	{LMID_STR("hard-reset"),		M202MD10B_HARD_RESET,		CommandLineHardReset},
	{LMID_STR("break"),			M202MD10B_HARD_RESET,		CommandLineHardReset},
	{LMID_STR("bright"),			M202MD10B_BRIGHT_4,		CommandLineBright},
	{LMID_STR("back"),			M202MD10B_BACK,			CommandLineBack},
	{LMID_STR("forward"),			M202MD10B_FORWARD,		CommandLineForward},
	{LMID_STR("clear-all"),			M202MD10B_CLEAR_ALL,		CommandLineOneChar},
	{LMID_STR("wipe"),			M202MD10B_CLEAR_ALL,		CommandLineOneChar},
	{LMID_STR("clear-all-goto-home"),	M202MD10B_CLEAR_ALL_GOTO_HOME,	CommandLineOneChar},
	{LMID_STR("cls"),			M202MD10B_CLEAR_ALL_GOTO_HOME,	CommandLineOneChar},
	{LMID_STR("goto-home"),			M202MD10B_GOTO_HOME,		CommandLineOneChar},
	{LMID_STR("home"),			M202MD10B_GOTO_HOME,		CommandLineOneChar},
	{LMID_STR("round-trip-mode"),		M202MD10B_ROUND_TRIP_MODE,	CommandLineOneChar},
	{LMID_STR("round"),			M202MD10B_ROUND_TRIP_MODE,	CommandLineOneChar},
	{LMID_STR("reset-blink-show-cursor"),	M202MD10B_RESET_BLINK_SHOW_CURSOR, CommandLineOneChar},
	{LMID_STR("show"),			M202MD10B_RESET_BLINK_SHOW_CURSOR, CommandLineOneChar},
	{LMID_STR("scroll-mode"),		M202MD10B_SCROLL_MODE,		CommandLineOneChar},
	{LMID_STR("scroll"),			M202MD10B_SCROLL_MODE,		CommandLineOneChar},
	{LMID_STR("hide-cursor"),		M202MD10B_HIDE_CURSOR,		CommandLineOneChar},
	{LMID_STR("hide"),			M202MD10B_HIDE_CURSOR,		CommandLineOneChar},
	{LMID_STR("blink-cursor"),		M202MD10B_BLINK_CURSOR,		CommandLineOneChar},
	{LMID_STR("blink"),			M202MD10B_BLINK_CURSOR,		CommandLineOneChar},
	{LMID_STR("under-line-cursor"),		M202MD10B_UNDER_LINE_CURSOR,	CommandLineOneChar},
	{LMID_STR("under"),			M202MD10B_UNDER_LINE_CURSOR,	CommandLineOneChar},
	{LMID_STR("block-cursor"),		M202MD10B_BLOCK_CURSOR,		CommandLineOneChar},
	{LMID_STR("block"),			M202MD10B_BLOCK_CURSOR,		CommandLineOneChar},
	{LMID_STR("reverse-cursor"),		M202MD10B_REVERSE_CURSOR,	CommandLineOneChar},
	{LMID_STR("reverse"),			M202MD10B_REVERSE_CURSOR,	CommandLineOneChar},
	{LMID_STR("define-char"),		M202MD10B_DEFINE_CHAR,		CommandLineDefineChar},
	{LMID_STR("define"),			M202MD10B_DEFINE_CHAR,		CommandLineDefineChar},
	{LMID_STR("goto-position"),		M202MD10B_GOTO_POSITION,	CommandLineGotoPosition},
	{LMID_STR("goto"),			M202MD10B_GOTO_POSITION,	CommandLineGotoPosition},
	{LMID_STR("dev"),			M202MD10B_PATH,			CommandLinePath},
	{LMID_STR("device"),			M202MD10B_PATH,			CommandLinePath},
	{LMID_STR("port"),			M202MD10B_PATH,			CommandLinePath},
	{LMID_STR("path"),			M202MD10B_PATH,			CommandLinePath},
	{LMID_STR("wait"),			M202MD10B_CWAITMS,		CommandLineCWaitMs},
	{LMID_STR("baud"),			M202MD10B_BAUD,			CommandLineBaud},
	{LMID_STR("rtscts"),			M202MD10B_RTSCTS,		CommandLineRtsCts},
	{LMID_STR("help"),			M202MD10B_HELP,			CommandLineHelp},
	{0, NULL, M202MD10B_HELP, CommandLineHelp},
};

typedef struct {
	const char		Option;
	char			Send;
	CommandLineOptionFn	*Fn;
} ShortMatch;

ShortMatch ShortMatchTable[] = {
	{'R', M202MD10B_HARD_RESET,		CommandLineHardReset},
	{'a', M202MD10B_BRIGHT_4,		CommandLineOneChar},
	{'b', M202MD10B_BRIGHT_3,		CommandLineOneChar},
	{'c', M202MD10B_BRIGHT_2,		CommandLineOneChar},
	{'d', M202MD10B_BRIGHT_1,		CommandLineOneChar},
	{'h', M202MD10B_BACK,			CommandLineBack},
	{'i', M202MD10B_FORWARD,		CommandLineForward},
	{'j', M202MD10B_CLEAR_ALL,		CommandLineOneChar},
	{'l', M202MD10B_CLEAR_ALL_GOTO_HOME,	CommandLineOneChar},
	{'m', M202MD10B_GOTO_HOME,		CommandLineOneChar},
	{'q', M202MD10B_ROUND_TRIP_MODE,	CommandLineOneChar},
	{'r', M202MD10B_SCROLL_MODE,		CommandLineOneChar},
	{'s', M202MD10B_RESET_BLINK_SHOW_CURSOR, CommandLineOneChar},
	{'t', M202MD10B_HIDE_CURSOR,		CommandLineOneChar},
	{'u', M202MD10B_BLINK_CURSOR,		CommandLineOneChar},
	{'v', M202MD10B_UNDER_LINE_CURSOR,	CommandLineOneChar},
	{'w', M202MD10B_BLOCK_CURSOR,		CommandLineOneChar},
	{'x', M202MD10B_REVERSE_CURSOR,		CommandLineOneChar},
	{'z', M202MD10B_DEFINE_CHAR,		CommandLineDefineChar},
	{'[', M202MD10B_GOTO_POSITION,		CommandLineGotoPosition},
	{'B', M202MD10B_BAUD,			CommandLineBaud},
	{'C', M202MD10B_RTSCTS,			CommandLineRtsCts},
	{'W', M202MD10B_CWAITMS,		CommandLineCWaitMs},
	{'F', M202MD10B_PATH,			CommandLinePath},
	{'H', M202MD10B_HELP,			CommandLineHelp},
	{0,0, NULL},
};

int CommandLineParseShort(CommandLine *cl, int *taken)
{	ShortMatch	*sm;
	const char	*p;
	char		c;
	char		so;
	int		result;
	int		taken_dummy;

	if (!taken) {
		taken = &taken_dummy;
	}

	p = cl->OptShort;
	if (!p) {
		return 0;
	}
	c = *p;
	sm = ShortMatchTable;
	while ((so = sm->Option) != '\x0') {
		if (so == c) {
			*taken = 1;
			result = sm->Fn(cl, sm->Send);
			return result;
		}
		sm++;
	}
	fprintf(stderr,
		"%s: %s: Unknown short option.\n",
		*(cl->Argv0),
		*(cl->Argv)
	);
	*taken = 1;
	cl->Argc--;
	cl->Argv++;
	return 0;
}

int CommandLineParseLong(CommandLine *cl, int *taken)
{	LongMatch	*lm;
	const char	*p;
	const char	*p2;
	ssize_t		l;
	ssize_t		ll;
	const char	*lo;
	char		c;
	int		result;
	int		taken_dummy;

	if (!taken) {
		taken = &taken_dummy;
	}

	p = cl->OptLong;
	if (!p) {
		return 0;
	}
	p2 = StrSkipToChar(p, '=');
	l = p2 - p;
	c = *p;
	lm = LongMatchTable;
	while ((ll = lm->Length) > 0) {
		if (ll != l) {
			lm++;
			continue;
		}
		lo = lm->Option;
		if (!lo) {
			break;
		}
		if (*lo != c) {
			lm++;
			continue;
		}
		if (memcmp(lo, p, l) != 0) {
			lm++;
			continue;
		}
		result = lm->Fn(cl, lm->Send);
		*taken = 1;
		return result;
	}
	fprintf(stderr,
		"%s: %s: Unknown long option.\n",
		*(cl->Argv0),
		*(cl->Argv)
	);
	*taken = 1;
	cl->Argc--;
	cl->Argv++;
	return 0;
}


int CommandLineParse(CommandLine *cl)
{	int		ret;
	int		result = 1;
	int		taken;

	while (!CommandLineEoa(cl)) {
		switch (cl->AState) {
		case AS_STR_ONLY:
			ret = CommandLineSendArg(cl, 0);
			if (!ret) {
				result = 0;
				goto done;
			}
			break;
		case AS_OPT_AND_STR:
			if (!CommandLineLongOptPrepare(cl)) {
				result = 0;
				goto done;
			}
			if (cl->OptDashDash) {
				cl->AState = AS_STR_ONLY;
				cl->Argc = cl->ArgcNextOpt;
				cl->Argv = cl->ArgvNextOpt;
				continue;
			}
			if (cl->OptDash) {
				ret = CommandLineSendStdIn(cl, 0);
				if (!ret) {
					result = 0;
					goto done;
				}
			}
			taken = 0;
			ret = CommandLineParseShort(cl, &taken);
			if (taken) {
				if (!ret) {
					result = 0;
					goto done;
				}
			} else {
				ret = CommandLineParseLong(cl, &taken);
				if (taken) {
					if (!ret) {
						result = 0;
						goto done;
					}
				}
			}
		default:
			fprintf(stderr,
				"%s: %s: Internal state error. Argv=%s\n",
				*(cl->Argv0),
				__func__,
				*(cl->Argv)
			);
			result = 0;
			goto done;
		}
	}
done:
	return result;
}

int MainB(CommandLine *cl)
{	int	ret;
	int	result = 1;
	speed_t	stBaud;
	int	bvalid = 0;

	cl->ParseOnly = 1;
	ret = CommandLineParse(cl);
	if (!ret) {
		return 0;
	}

	if (!(cl->Path)) {
		fprintf(stderr,
			"%s: Specify serial port path with -F path\n",
			*(cl->Argv0)
		);
		return 0;
	}

	ret = PortM202MD10BOpen(&(cl->Port), cl->Path);
	if (!ret) {
		return 0;
	}

	stBaud = BaudToSpeedT(cl->Baud, &bvalid);
	switch (stBaud) {
	case B1200:
	case B2400:
	case B4800:
	case B9600:
		break;
	default:
		fprintf(stderr, "%s: -B: Choose baud rate one of 1200, 2400, 4800, or 9600.\n",
			*(cl->Argv0)
		);
		result = 0;
		goto out_close;
	}

	ret = PortM202MD10BConfig(&(cl->Port), stBaud, cl->RtsCts, cl->CWaitMs);
	if (!ret) {
		result = 0;
		goto out_close;
	}

	CommandLineReset(cl);
	cl->ParseOnly = 0;
	ret = CommandLineParse(cl);
	if (!ret) {
		result = 0;
	}

out_close:
	ret = PortM202MD10BClose(&(cl->Port));
	if (!ret) {
		result = 0;
	}
	return result;
}

int main(int argc, char **argv, char **env)
{	int		ret;
	int		result = 0;
	CommandLine	*cl;

	cl = &CLine;
	if (!CommandLineInit(cl, argc, argv)) {
		result = 3;
		goto out;
	}
	ret = MainB(cl);
	if (!ret) {
		result = 1;
	}
	CommandLineDone(cl);
out:
	return result;
}
