#!/usr/bin/perl
use re 'eval';

open(F, $ARGV[0]);
$BR = qr{ \[ ( (?: (?> [^\[\]]+ )| (??{ $BR }) )* ) \] }x;
$br = qr{ \{  (?: (?> [^{}]+ )	| (??{ $br }) )* \} }x;

$sups="\\\\raise.5ex\\\\hbox\{\\\\small($br)\}";
$subs="\\\\lower.5ex\\\\hbox\{\\\\small($br)\}";
$em="\\\\ensuremath";
$EM="\\ensuremath";
$b="\\\\textbf";
$B="\\textbf";
$MIT="\\mathit";
$emem="$em($br)$em($br)";
$dummyem="$em\\{\\_\\{\\}\\}";
$derive="$em\\{\\\\vdash\\}";
$models="$em\\{\\\\models\\}";
$deriveSp="($derive)$em\\{\\_($br)\\}";
$modelSp="($models)$em\\{\\_($br)\\}";
#Unfortunately, the following two cases need different spacings
$EqSp1="(=)$em\\{\\_(\\{$em\\{\\\\blacktriangledown\\}\\})\\}";
$EqSp2="(=)$em\\{\\_(\\{$em\\{\\\\bigtriangledown\\}\\})\\}";
$pcst="$em\\{\\\\hookrightarrow\\}";
$pcstSp="($pcst)$em\\{\\^($br)\\}";
$bigcup="\\\\textbf\\{$em\\{\\\\cup\\}\\}";
$bigcap="\\\\textbf\\{$em\\{\\\\cap\\}\\}";
$emset="$em($br)";
$plural1="$emset$em\\{(\\_$br)?\\^\\{\\*\\}(\\_$br)?\\}";
$plural2="$em\\{($br)\\{(\\_$br)?\\^\\{\\*\\}(\\_$br)?\\}\\}";
$plural3="\\{($BR)\\}$em\\{\\^\\{\\*\\}\\}";
$pluralbad="$em\\{\\^\\{\\*\\}\\}";

while(!eof(F)) {    
    $l = <F>;
    
    do {
	$ol = $l;
	
	#Subscript / Superscript handling
	$l =~ s/$sups$subs/$EM\{^$1_$2\}/g;
	$l =~ s/$subs$sups/$EM\{_$1^$2\}/g;
	$l =~ s/$sups/$EM\{^$1\}/g;
	$l =~ s/$subs/$EM\{_$1\}/g;
	$l =~ s/$dummyem//g;
	
	#Bold-Ops - Big-Ops
	$l =~ s/$bigcup/$EM\{\\bigcup\}/g;
	$l =~ s/$bigcap/$EM\{\\bigcap\}/g;
	
        #{| to {\!| and |} to |\!}
	# Note that this is *not* paranthesizing replacement
	$l =~ s/\{\\\{\}\{\\textbar\}/$EM\{\\\{\\!\|\}/g;
	$l =~ s/\{\\textbar\}\{\\\}\}/$EM\{\|\\!\\\}\}/g;

	# \textbf{*}letter\textbf{*} to \mathbb{letter}
	$l =~ s/$b\{\*\}([A-Za-z])\\textbf\{\*\}/$EM\{\\mathbb{$1}\}/g;
	
	# \emph{*}letter\emph{*} to \mathcal{letter}
	$l =~ s/\\emph\{\*\}([A-Za-z])\\emph\{\*\}/$EM\{\\mathcal{$1}\}/g;

	# \textbf{\emph{*}}letter\textbf{\emph{*}} to \mathfrak{letter}
	$l =~ s/\\textbf\{\\emph\{\*\}\}([A-Za-z])\\textbf\{\\emph\{\*\}\}/$EM\{\\mathfrak{$1}\}/g;

	# \textbf{\emph{*}}letter\textbf{\emph{*}} to \mathfrak{letter}
	$l =~ s/\\textbf\{\\emph\{\*\}\}([A-Za-z])\\textbf\{\\emph\{\*\}\}/$EM\{\\mathfrak{$1}\}/g;

	# \textbf{\emph{\?}}text\textbf{\emph{\?}} to \ensuremath{\mathrm{text}}
	$l =~ s/\\textbf\{\\emph\{\?\}\}([A-Za-z0-9]+)\\textbf\{\\emph\{\?\}\}/$EM\{\\mathrm\{$1\}\}/g;

	#OK??
	#Derivation and Modelling
	$l =~ s/$deriveSp/\\mbox\{$1\\hskip -0.9ex\\raise0.25ex\\hbox\{$EM\{\_\{\_$MIT\{$2\}\}\}\}\}/g;
	$l =~ s/$modelSp/\\mbox\{$1\\hskip -1.2ex$EM\{\_\{_$MIT\{$2\}\}\}\}/g;
	#Transitive Right execution
	#$l =~ s/$transR/$EM\{\\Rightarrow\}\\hskip -2.1ex\{\*\}/g;

	#Equality onder another operator
	$l =~ s/$EqSp1/\\mbox\{$1\\hskip -1.2ex\\lower 0.1ex\\hbox\{$EM\{\_\{\_\{$2\}\}\}\}\}/g;
	$l =~ s/$EqSp2/\\mbox\{$1\\hskip -1.4ex\\raise 0.1ex\\hbox\{$EM\{\_\{\_\{$2\}\}\}\}\}/g;
	
	#Polymorphic constraint
	$l =~ s/$pcstSp/\\mbox\{$1\\hskip -2ex$EM\{\^\{$MIT\{$2\}\}\}\}/g;
	
	#Plural handling
	#FIX: Deal with parentheses
	$l =~ s/$plural1/$EM\{\\overline\{$1$2$3\}\}/g;
	$l =~ s/$plural2/$EM\{\\overline\{$1$2$3\}\}/g;
	$l =~ s/$plural3/$EM\{\\overline\{\{$1\}\}\}/g;

	# \ensuremath{} redundancy elimination
	$l =~ s/$emem/\\ensuremath{$1$2}/g;
	
	# Worst hack to fix \\ in the beginning of a line to \vspace(4pt}
	# The correct thing to do is to add a @latex.ptsz arrtibute to 
	# OSDOC's <br>
	$l =~ s/^\\\\/\\vspace\{4pt\}/g;
	
    } while($ol ne $l);
    
    #This pass may not really be desirable
    do {
	$ol = $l;
	
	# I am not checking \ensuremath{}, careful.
	$l =~ s/_\{([a-z]+)([0-9]+)\}/\_\{$1\_\{$2\}\}/g;
	$l =~ s/\^\{([a-z]+)([0-9]+)\}/\^\{$1\_\{$2\}\}/g;
    } while($ol ne $l);

    #This pass may not really be necessary    
    do {
	$ol = $l;
	$l =~ s/$emem/\\ensuremath\{$1$2\}/g;
    } while($ol ne $l);

    if($l =~ /($pluralbad)/) {
        print STDERR "Bad plural usage: $1::$l";
    }  
    
    print $l;
}    
close(F);

