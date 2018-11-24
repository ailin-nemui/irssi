#!/usr/bin/perl
use strict; use warnings;

while (<>) {
    my $args = y/,//;
    s{^\s* " ([^"]*?)(\s)? "(?:<([^>]+)>)? }{SIGNAL_REGISTER@{[
	$2 ? "_" : ""
    ]}(@{[ $1 =~ s/_/__/gr =~ s/ /_/gr ]}, $args@{[
	$2 ? ", const char *\L$3" : ""]}}gx ;
    my %X;
    s{ (\w+)_REC(,|$) }{$1_REC *\L$1@{[ $X{"\L$1"}++ || "" ]}$2}gx ;
    s{ CONFIG_(\w+)(,|$) }{CONFIG_$1 *\L$1@{[ $X{"\L$1"}++ || "" ]}$2}gx ;
    /\(/ && s/$/)/ ;
    s/ (u?)int (?![*])/ $1int_in_ptr /g;
    s{ (SIGNAL_REGISTER(_)?\(\w+,\ (\d+))(,\ (.*?))?\) }{$1, (@{[ $4 ? $5 : "void" ]})@{[
	$4
	    ? ",\n\t". $5 =~ s{[^,]* [*]*(\w+)\s*(?:/\*.*?\*/\s*)*(,|$)}{ $1$2}gr
	    : ""
    ]})}gx ;
    s{ SIGNAL_REGISTER_\(\w+,\ \d+,\ \([^,]+(?:,\ (.*?))?\),\K}{\n\t(@{[ $1 ? $1 : "void" ]}),}gx ;
}
continue {
    print;
}
