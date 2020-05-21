#pragma once

#include <ntifs.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static const POOL_TYPE poolTypeUsed = PagedPool;

/**
* Virtual Machine Style Regular Expression parsing/NFA emulation
* used for wildcard matching. O(nm) algorithm.
*
* See http://swtch.com/~rsc/regexp/ for more on efficient RegEx parsing.
*
* Copyright (c) March 29, 2013 Paul Frischknecht
* Can be distributed under the MIT licence, see bottom of file.
*/

//#define wildcard_fast_DEBUG /* define to make the alogorithm printf what its doing */

/**
* Instructions are:
*
* star
*   This launches a new thread at the current position and continues with the
*   current thread at the next position accepting any character.
* char c
*   Accepts exactly the character c
* anychar
*   Accepts any character.
* end
*   Accepts the end of the input string.
*/
enum InstructionType {
	End = 0,
	Star = 1,
	Char = 2,
	AnyChar = 3
};

struct Instruction {
	InstructionType i;
	int c;       /* the caracter this instruction matches - undefined for non Char */
	int threads; /* threads active in the given instruction */
	/*
	* storing this here allows us to find out wether there is a thread
	* active at a given instruction in O(1)
	*/
};

/**
* Wildcard (file path) matching.
* See for example
* http://www.microsoft.com/resources/documentation/windows/xp/all/proddocs/en-us/find_c_search_wildcard.mspx?mfr=true
*
*  * matches any amount of any characters,
*  ? matches any single character.
*  c matches c.
*
* Escaping of '*', '?' and '\' via '\*', '\?' and '\\' implemented.
* All other bytes are recognized directly as is.
* The string may not end in a single (uneven amount of) '\'.
*
* If text_end is 0, the text is assumed to be 0 terminated.
* Same for pattern_end. The end is exclusive.
*
* @return A pointer to the character after the last character matched.
*
* Virtual machine O(nm) implementation, see http://swtch.com/~rsc/regexp/regexp2.html
*
* TODO Factor out the precompilation step to speed up matching multiple strings
* against the same expression.
*/
template<class Pattern, class Text>
Text wildcard_fast(
	Pattern const pat_begin,
	Pattern const pat_end,
	Text const text_begin,
	Text const text_end)
{
	/*
	* give some reasonable default program size so we don't have to call
	* malloc in most cases
	*/
#define DEFAULTonstack_program_SIZE 256
	Instruction onstack_program[DEFAULTonstack_program_SIZE];
	/* this stores the current run and next run thread list, alternating */
	/* there are as most as many threads as instructions */
	Instruction* onstack_threads[DEFAULTonstack_program_SIZE * 2];

	Instruction** threads = onstack_threads;
	Instruction*  program = onstack_program;
	int           program_size = sizeof(onstack_program) / sizeof(*program);

	Instruction* program_last_instruction = program + program_size - 1;

	/* program and pattern pointers */
	Instruction* pp = program;
	Pattern      patp = pat_begin;

	/* compile */

	while ((pat_end == 0 && *patp != 0) || (pat_end != 0 && patp != pat_end)) {

		/* need more space */
		if (pp == program_last_instruction) {

			Instruction*  old_program = program;
			Instruction** old_threads = threads;
			int old_program_size = program_size;

			program_size *= 2;

			program = (Instruction*)ExAllocatePoolWithTag(poolTypeUsed, program_size*sizeof(*program), 'dlWX');
			ASSERT(program);

			threads = (Instruction**)ExAllocatePoolWithTag(poolTypeUsed, program_size*sizeof(*threads) * 2, 'dlWX');
			ASSERT(threads);

			memcpy(program, old_program, old_program_size*sizeof(*program));

			if (old_program != onstack_program) {
				ExFreePool(old_program); ExFreePool(old_threads);
			}

			program_last_instruction = program + program_size - 1;
			pp = pp - old_program + program;
		}

		/* parse pattern */
		switch (*patp) {
		case '*':
			pp->i = Star;
			/* Optimize multiple stars away */
			while ((pat_end == 0 || patp + 1 != pat_end) && *(patp + 1) == '*')
				patp++;
			break;

		case '?':
			pp->i = AnyChar;
			break;

		case '\\':
			pp->i = Char;
			pp->c = *(++patp); /* assumes string does not end in \ */
			break;

		default:
			pp->i = Char;
			pp->c = *patp;
			break;
		}

		pp->threads = 0;

		pp++;
		patp++;
	}

	/* add the End instruction at the end */
	program_last_instruction = pp;
	pp->i = End;
	pp->threads = 0;

	/* run */
	Text sp = text_begin; /* input string pointer */
	int n = 1, c = 0; /* next and current index */
	int threadcount[2];

	/* initialize */
	threadcount[c] = 1;
	threads[0] = program;
	threads[0]->threads++;

	/* run over text */
	while ((text_end == 0 && *sp != 0) || (text_end != 0 && sp != text_end)) {
		/* unless recreated, all threads will die */
		threadcount[n] = 0;

		/* run over threads */
		for (int i = 0; i < threadcount[c]; i++) {

			Instruction* inst = threads[2 * i + c];
			switch (inst->i) {
			case End:
				/* we may not reach end early */
				/* kill this thread without recrating it */
				inst->threads--;
				continue; /* with for loop */

			case Char:
				if (*sp != inst->c) {
					/* if the character is not matched, kill this thread */
					inst->threads--;
					continue;
				}
				break;

			case Star:
				/* spawn off additional thread at current location */

				if (inst->threads == 1) {
					/* only if there's noone active there yet */
					threads[2 * (threadcount[n]++) + n] = inst;
					inst->threads++;
				}
				break;
			}
			/*
			* common actions: increase program counter for current thread,
			* decrese amount of threads in last (current) instruction.
			*/
			inst->threads--;
			inst++;
			inst->threads++;

			/* respawn at new location in next iteration */
			threads[2 * (threadcount[n]++) + n] = inst;

			if (inst->i == Star && (inst + 1)->threads == 0) {
				/*
				* already follow no-match option to give us
				* more stuff to do
				*/
				threads[2 * (threadcount[n]++) + n] = inst + 1;
				(inst + 1)->threads++;
			}

#ifdef wildcard_fast_DEBUG
			for (int i = 0; i < threadcount[n]; i++) {
				printf("thread %d at %d.\n", i, threads[2 * i + n] - program);
			}
#endif

		}

#ifdef wildcard_fast_DEBUG
		const char *ns[] = {
			"end",
			"star",
			"char",
			"anychar",
		};
		for (Instruction* p = program; p->i; p++) {
			printf("%d. %s %c (%d threads)\n", p - program, ns[p->i], p->i == Char ? p->c : ' ', p->threads);
		}
#endif

		/* swap next and current and advance */
		n = c;
		c = !c;
		sp++;
	}

	/*
	* if there is no thread active in the End instruction when the
	* end of the input was reached, this was no match
	*/
	if (program_last_instruction->threads == 0) sp = 0;

	if (program != onstack_program) {
		/* only need to ExFreePool if we used ExAllocatePoolWithTag */
		ExFreePool(program); ExFreePool(threads);
	}

	return sp;
}

bool WildcardFast(const char* pattern, const char* text)
{
	return (wildcard_fast(pattern, (const char*)0, text, (const char*)0) != (const char*)0);
}

bool WildcardFast(const wchar_t* pattern, const wchar_t * text)
{
	return (wildcard_fast(pattern, (const wchar_t*)0, text, (const wchar_t*)0) != (const wchar_t*)0);
}