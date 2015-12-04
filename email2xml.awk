#!/usr/bin/gawk -f

function after(str,A)     { return match(str,A) ? substr(str,RSTART+RLENGTH) : "" }
function before(str,Z)    { return match(str,Z) ? substr(str,1,RSTART-1) : "" }
function between(str,A,Z) { return before(after(str,A),Z) }

# function between(str,beg,end) {
#     if (!match(str,beg)) return "";
#     str = substr(str,RSTART+RLENGTH);
#     if (!match(str,end)) return "";
#     return substr(str,1,RSTART-1);
# }

function time(str, cmd) { # Thu, 14 Sep 2006 05:36:09 -0700   -->   1158237369
    # sub("-[0-9]+$","",str); # remove trailing timezone ... why?
    gsub("\"","",str); # some timezones quoted
    cmd = "date -d '"str"' '+%s'";
    cmd | getline;
    close(cmd);
    return $0;
}

function dump(   HEAD,BODY,EOM,id,date,from,to,subj,repl,refs,text,type) {
    IGNORECASE = 1;
    
    gsub("[\t\r]"," ",DOC);
    gsub("\n\n","\n\r\n",DOC); # mark every empty line with a \r
    
    HEAD = before(DOC,"\r"); # header is the part before the empty line
    gsub ("\n "," ",HEAD); # put multi-line fields all on one line
    id   = between(HEAD,"\nMessage-ID:  *<?", "[> \n]");
    date = between(HEAD,"\nDate: ",       "\n");
    from = between(HEAD,"\nFrom: ",       "\n");
    to   = between(HEAD,"\nTo: ",         "\n");
    subj = between(HEAD,"\nSubject: ",    "\n");
    repl = between(HEAD,"\nIn-Reply-To: ","\n"); repl = between(repl,"<",">");
    refs = between(HEAD,"\nReferences: ", "\n");
    refs = id" "repl" "refs; 
    gsub("[^a-z0-9 ]","",refs);
    
    gsub("\nContent-Type:","\t&",DOC); DOC=DOC"\t"; # terminate every section with a tab
    
    if      (text = between(DOC,"\nContent-Type: text/plain","\t")) type = "text";
    else if (text = between(DOC,"\nContent-Type: text/html", "\t")) type = "html";
    else if (text = between(DOC,"\nContent-Type: text",      "\t")) type = "warn";
    else if (!index(DOC,"\nContent-Type:"))           { text = DOC; type = "body"; }
    else                                                            type = "none";
    text = after(text,"\r"); # text comes after first empty line
    gsub("\r[^\r]*$","",text); # remove junk after last empty line
    if (type == "html") gsub("<[^>]*>"," ",text);
    gsub("[<>]","_",text); # could interfere with our tags
    gsub("[\t\r ]+"," ",text); # remove runs of whitespace
    gsub("\n[ \n]*\n","\n\n",text); # remove empty lines 
    
    if (id) {
	printf "<DOC id=\"%s\" time=%d type=%s>\n", id, time(date), type;
	print "Date:", date;
	print "From:", from;
	print "To:", to;
	print "Subject:", subj;
	print "";
	print text;
	print "<refs>", refs, "</refs>"; # includes id & repl
	if (repl) printf "<src id=\"%s\">\n", repl;
	print "</DOC>\n";
    }
    else print "SKIPPING:", date, subj > "/dev/stderr"; 
    
    IGNORECASE = 0;
}

/^From / {  # mbox beginning of new email
    if (file) dump(); 
    if (file != FILENAME) print FILENAME > "/dev/stderr";
    file = FILENAME; 
    DOC = ""; 
}

file { DOC = DOC"\n"$0; }

END { if (file) dump(); }
    
#     TXT = REF = part = stop = 0;
#     DOC = HDR = 1;
#     file = FILENAME; gsub("[ \t\r\n]","",file);
#     date = from = to = subj = id = inrt = refs = text = "";
# }

# HDR && /^Date: /        { $1=""; date = $0; }
# HDR && /^From: /        { $1=""; from = $0; }
# HDR && /^To: /          { $1="";   to = $0; }
# HDR && /^Subject: /     { $1=""; subj = $0; }
# HDR && /^Message-ID: /  {   id = $2; }
# HDR && /^In-Reply-To: / { inrt = $2; }
# HDR && /^References: /  { refs = $2; REF = 1; }
# HDR && REF && /^ +</    { refs = refs" "$0; }
# HDR && REF && /^[^ ]/   { REF = 0; }

# / boundary=/ { stop=$0; sub(".*boundary=","",stop); gsub("\"","",stop); }

# HDR && /^Content-Type: /      { TXT = 0; }
# HDR && /^Content-Type: text/  { TXT = 1; ++part; }
# HDR && TXT && /^$/            { HDR = 0; }
# TXT && stop && index($0,stop) { TXT = 0; }
# /^%!PS-Adobe/                 { TXT = 0; }
# /Content-.*-Encoding: base64/ { TXT = 0; }
# TXT && !HDR && part == 1      { text = text" "$0; }


# BEGIN { split("Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec",a); for (i in a) month[a[i]] = i; }
# /<DOC/ || /<\/DOC>/ {gsub("[<>]","_")}
# /^((>From)|(Received:)) / {HDR=1; text=0;}
