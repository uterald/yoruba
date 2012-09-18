// yoruba_inu.cpp  (c) Douglas G. Scofield, Dept. Plant Physiology, Umeå University
//
// Inu (English command is contents) summarizes the contents of a BAM file.
//
// Inu reads the BAM file structure and summarizes the header, references and read
// contents.  It can also quietly check the validity of the header, and print raw
// header contents
//
// Inu is the Yoruba (Nigeria) noun for 'inside'.
//
// Uses BamTools C++ API for handling BAM files


// CHANGELOG
//
//
//
// TODO
//
// Update README.md
// Update with options
//

#include "yoruba_inu.h"

using namespace std;
using namespace BamTools;
using namespace yoruba;

static string       input_file;  // defaults to stdin, set from command line
static int64_t      opt_reads_to_report = 10;
static bool         opt_quit = false;
static bool         opt_quiet = false;
static bool         opt_raw = false;
static int32_t      opt_refs_to_report = 10;
static int32_t      opt_raw_to_report = 1000;
#ifdef _WITH_DEBUG
static int32_t      opt_debug = 0;
static int64_t      opt_reads = -1;
static int64_t      opt_progress = 0; // 100000;
#endif


//-------------------------------------


#ifdef _STANDALONE
int 
main(int argc, char* argv[]) {
    return main_inu(argc, argv);
}
#endif


//-------------------------------------


static int
usage()
{
    cerr << endl;
    cerr << "Usage:   " << YORUBA_NAME << " contents [options] <in.bam>" << endl;
    cerr << "         " << YORUBA_NAME << " inu [options] <in.bam>" << endl;
    cerr << endl;
    cerr << "Either command invokes this function." << endl;
    cerr << endl;
    cerr << "\
Summarizes the contents of the BAM file <in.bam>.\n\
\n\
Output includes:\n\
   (1) header lines exclusive of the reference sequences (@SQ lines)\n\
   (2) a summary of reference sequences, if there are more than " << opt_refs_to_report << ",\n\
       otherwise all @SQ lines are printed\n\
   (3) a summary of read content\n\
\n";
    cerr << "\
Options: --reads-to-report INT  print this many reads [" << opt_reads_to_report << "]\n\
         --refs-to-report INT   print this many references [" << opt_refs_to_report << "]\n\
         --quit                 quit early, don't count all reads\n\
         --quiet                don't print any details, only check validity; combine\n\
                                with --raw to only check the header and print raw lines\n\
         --raw                  print raw header contents\n\
         --raw-to-report INT    number of --raw header characters to print [" << opt_raw_to_report << "]\n\
         --? | -? | --help      longer help\n\
\n";
#ifdef _WITH_DEBUG
    cerr << "\
         --debug INT     debug info level INT [" << opt_debug << "]\n\
         --reads INT     only process INT reads [" << opt_reads << "]\n\
         --progress INT  print reads processed mod INT [" << opt_progress << "]\n\
\n";
#endif
    cerr << "Inu is the Yoruba (Nigeria) noun for 'inside'." << endl;
    cerr << endl;

    return 1;
}


//-------------------------------------


int 
yoruba::main_inu(int argc, char* argv[])
{
    //----------------- Command-line options

	if( argc < 2 ) {
		return usage();
	}

    enum { OPT_reads_to_report, OPT_refs_to_report, OPT_raw, OPT_quit,
        OPT_quiet, OPT_raw_to_report,
#ifdef _WITH_DEBUG
        OPT_debug, OPT_reads, OPT_progress,
#endif
        OPT_help };

    CSimpleOpt::SOption inu_options[] = {
        { OPT_refs_to_report,  "--refs-to-report",  SO_REQ_SEP },
        { OPT_reads_to_report, "--reads-to-report", SO_REQ_SEP },
        { OPT_quit,            "--quit",            SO_NONE },
        { OPT_quiet,           "--quiet",           SO_NONE },
        { OPT_raw,             "--raw",             SO_NONE },
        { OPT_raw_to_report,   "--raw-to-report",   SO_REQ_SEP },
        { OPT_help,            "--?",               SO_NONE }, 
        { OPT_help,            "-?",                SO_NONE }, 
        { OPT_help,            "--help",            SO_NONE },
#ifdef _WITH_DEBUG
        { OPT_debug,           "--debug",           SO_REQ_SEP },
        { OPT_reads,           "--reads",           SO_REQ_SEP },
        { OPT_progress,        "--progress",        SO_REQ_SEP },
#endif
        SO_END_OF_OPTIONS
    };

    CSimpleOpt args(argc, argv, inu_options);

    while (args.Next()) {
        if (args.LastError() != SO_SUCCESS) {
            cerr << NAME << " invalid argument '" << args.OptionText() << "'" << endl;
            return usage();
        }
        if (args.OptionId() == OPT_help)       return usage();
        else if (args.OptionId() == OPT_reads_to_report) 
            opt_reads_to_report = strtoll(args.OptionArg(), NULL, 10);
        else if (args.OptionId() == OPT_refs_to_report) 
            opt_refs_to_report = strtol(args.OptionArg(), NULL, 10);
        else if (args.OptionId() == OPT_quit)  opt_quit = true;
        else if (args.OptionId() == OPT_quiet) opt_quiet = true;
        else if (args.OptionId() == OPT_raw)   opt_raw = true;
        else if (args.OptionId() == OPT_raw_to_report) 
            opt_raw_to_report = strtol(args.OptionArg(), NULL, 10);
#ifdef _WITH_DEBUG
        else if (args.OptionId() == OPT_debug) 
            opt_debug = args.OptionArg() ? atoi(args.OptionArg()) : opt_debug;
        else if (args.OptionId() == OPT_reads) 
            opt_reads = strtoll(args.OptionArg(), NULL, 10);
        else if (args.OptionId() == OPT_progress) 
            opt_progress = args.OptionArg() ? strtoll(args.OptionArg(), NULL, 10) : opt_progress;
#endif
        else {
            cerr << NAME << " unprocessed argument '" << args.OptionText() << "'" << endl;
            return 1;
        }
    }

    if (args.FileCount() > 1) {
        cerr << NAME << " requires at most one BAM file specified as input" << endl;
        return usage();
    } else if (args.FileCount() == 1) {
        input_file = args.File(0);
    } else if (input_file.empty()) {  // don't replace if not empty, a defauult is set
        input_file = "/dev/stdin";
    }

    //----------------- Open file, start reading data

	BamReader reader;

	if (! reader.Open(input_file)) {
        cerr << NAME << "could not open BAM input" << endl;
        return 1;
    }

    SamHeader header = reader.GetHeader();

    // with --quiet (and without --raw) this validity check is the primary operation
    if (! header.IsValid(true)) {
        cout << NAME << " header not well-formed, errors are:" << endl;
        cout << header.GetErrorString() << endl;
    }

    if (opt_raw) {
        const string header_printable = header.ToString();
        if (header_printable.length() > uint32_t(opt_raw_to_report)) {
            cout << NAME << " header string, first " << opt_raw_to_report 
                << " characters:" << endl;
            cout << header_printable.substr(0, opt_raw_to_report);
            cout << endl;
        } else {
            cout << NAME << " header string, complete contents:" << endl;
            cout << header_printable;
        }
    }

    if (opt_quiet) { // don't do any more
	    reader.Close();
	    return 0;
    }

    //----------------- Header metadata

    if (header.HasVersion() || header.HasSortOrder() || header.HasGroupOrder()) {
        cout << NAME << "[headerline]";
        if (header.HasVersion()) cout << " VN:'" << header.Version << "'";
        if (header.HasSortOrder()) cout << " SO:'" << header.SortOrder << "'";
        if (header.HasGroupOrder()) cout << " GO:'" << header.GroupOrder << "'";
        cout << endl;
    } else cout << NAME << "[headerline] no header line found" << endl;

    //----------------- Reference sequences

    RefVector refs;

    if (header.HasSequences()) {
        int32_t ref_count = reader.GetReferenceCount();
        // cout << NAME << "[ref] " << ref_count << " references found in the BAM file" << endl;
        if (ref_count > opt_refs_to_report)
            cout << NAME << "[ref] displaying the first " << opt_refs_to_report 
                << " reference sequences" << endl;
        refs = reader.GetReferenceData();
        for (int32_t i = 0; i < ref_count && i < opt_refs_to_report; ++i) {
            cout << NAME << "[ref] @SQ ID:" << i 
                << "\t" << "NM:" << refs[i].RefName 
                << "\t" << "LN:" << refs[i].RefLength
                << endl;
        }
    } else cout << NAME << "[ref] no reference sequences found" << endl;

    //----------------- Read groups

    if (header.HasReadGroups()) {
        string prefix = NAME "[readgroup] ";;
        printReadGroupDictionary(cout, header.ReadGroups, prefix);
    } else cout << NAME << "[readgroup] no read group dictionary found" << endl;

    //----------------- Programs

    if (header.HasPrograms()) {
        for (SamProgramConstIterator pcI = header.Programs.ConstBegin();
                pcI != header.Programs.ConstEnd(); ++pcI) {
            cout << NAME << "[program] @PG";
            cout << " ID:'" << (*pcI).ID << "'";
            cout << " PN:'" << (*pcI).Name << "'";
            cout << " CL:'" << (*pcI).CommandLine << "'";
            cout << " PP:'" << (*pcI).PreviousProgramID << "'";
            cout << " VN:'" << (*pcI).Version << "'";
            cout << endl;
        }
    } else cout << NAME << "[program] no program information found" << endl;


    //----------------- Comments

    if (! header.Comments.empty()) {
        for (vector<string>::const_iterator vI = header.Comments.begin();
                vI < header.Comments.end(); ++vI) {
            cout << NAME << "[program] @CO '" << (*vI) << "'" << endl;
        }
    } else cout << NAME << "[comment] no comment lines found" << endl;

    //----------------- Reads

	BamAlignment al;  // holds the current read from the BAM file

    int64_t n_reads = 0;  // number of reads processed

    if (opt_reads_to_report) {
        cout << NAME << "[read] printing the first " << opt_reads_to_report << " reads" << endl;
    }

	while (reader.GetNextAlignmentCore(al) && (opt_reads < 0 || n_reads < opt_reads)) {

        ++n_reads;

        if (n_reads <= opt_reads_to_report) {
            al.BuildCharData();
            cout << NAME << "[read] ";
            printAlignmentInfo(cout, al, refs, 99);
        }

        if (opt_progress && n_reads % opt_progress == 0)
            cerr << NAME << "[read] " << n_reads << " reads processed..." << endl;

        if (opt_quit && n_reads == opt_reads_to_report)
            break;
	}

    cout << NAME << "[read] " << n_reads << " reads examined from the BAM file" << endl;

	reader.Close();

	return 0;
}

