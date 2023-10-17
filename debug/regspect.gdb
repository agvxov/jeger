define regspect
	delete
	break regex_match
	ignore 1 $arg0 - 1
	run
	tui refresh
	print * regex
end
