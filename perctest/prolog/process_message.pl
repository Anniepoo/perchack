:- module(process_message, [process_message/3, start_state/1]).
/** <module> Process raw perc frames from the camera

*/

:- use_module(recognize).

%%	process_message(+Frame:frame, +State:list, -NewState:list) is
%	det
%
%	process a single frame from the camera, returning the new state
%
%
/*
process_message(frame([0]), [empty(N)|T], [empty(NN)|T]) :-
	NN is N + 1.
process_message(frame([0]), T, [empty(1)|T]).
*/
process_message(Frame, State, [Frame|Upto299]) :-
	take(299, State, Upto299),
	debug(perc, '~w', [Frame]),
	recognize_gestures([Frame|Upto299]).

start_state([]).

take(0, _, []).
take(_, [], []).
take(N, [H|T], [H|TO]) :-
	NN is N - 1,
	take(NN, T, TO).

