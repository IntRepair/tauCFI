all: texclean
	latexmk -pdf

.SECONDARY:

texclean:
	rm -f *.aux
	rm -f *.fdb_latexmk
	rm -f *.fls
	rm -f *.log
	rm -f *.nav
	rm -f *.out
	rm -f *.pdf
	rm -f *.snm
	rm -f *.toc
	rm -f *.vrb
	rm -f *.synctex.gz


clean: texclean
