:- module(lang_def, []).

:- use_module(library(http/dcg_basics)).

:- multifile gdl:gdl_def/2.
:- dynamic gdl:gdl_def/2.

gdl --> b, [].
gdl -->
	b, pose_definition,b,
	gdl.
gdl -->
	gesture_definition,b,
	gdl.

gesture_definition -->
	"gesture",
	b,name(Name),b,
	":",b,
	gesture_segment_list(SegmentList),b,
	".",
	{
	    assertz(gdl:gdl_def(Name, SegmentList))
	}.

gesture_segment_list([]) --> [].
gesture_segment_list(H) --> gesture_segment(H).
gesture_segment_list([H|T]) -->
	gesture_segment(H),b,
	";",b,
	gesture_segment_list(T).

%  TODO generate code
gesture_segment(is_pose_on_frame(Name)) -->
	"pose",bb,
	name(Name).

name(Name) -->
    first_char_name(H),
    rest_name(T),
    {
        atom_codes(Name, [H|T])
    }.

first_char_name(H) -->
	[H],
	{
	    code_type(H, csymf)
	}.
rest_name([]) --> [].
rest_name([H|T]) -->
	[H],
	{
	   code_type(H, csym)
	},
	rest_name(T).


b --> blanks.

bb --> " ",b.




