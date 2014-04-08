/**
 * STP: This is clearly a cpp file. It used to be named *.c.
 *
 */

#include <cstdlib>
#include <stdio.h>
#include <time.h>
#include <algorithm>
#include <ext/algorithm>
#include <math.h>
#include <iostream>

#include <cstring>
#include "macrodefine.h"
#include "readfile.h"
#include "stringhandle.h"

#include <cstdlib>

#include <fstream> // for printing edges between core nodes in expansion network
using std::swap;
using std::cout;
using std::cerr;
using std::endl;
using std::sort;


bool keep_SSRs = false;
bool print_clouds_in_regions = false;
bool print_legacy_regions = false;
bool expand_recursively = false;

#include <deque> // For clouds in regions

static int sbsearch2(int n, const cloud_type2 *argv, const unsigned long& key) {
	int m;
	int site = 0;

	while (n >= 1) {
		m = n / 2;
		if (argv[site + m].index == key)
			return (site + m);
		else if (argv[site + m].index > key)
			n = m;
		else {
			site = site + m + 1;
			n = n - m - 1;
		}
	}

	return (-1);
}

//STP: Is this a binary search algorithm?
static int sbsearch3(int nOligos, const cloud_type3 *oligos,
		const unsigned long& key) {
	int m;
	int site = 0;

	while (nOligos >= 1) {
		m = nOligos / 2;
		if (oligos[site + m].number_pattern == key)
			return (site + m);
		else if (oligos[site + m].number_pattern > key)
			nOligos = m;
		else {
			site = site + m + 1;
			nOligos = nOligos - m - 1;
		}
	}

	return (-1);
}

static bool highnumber3(cloud_type3 a, cloud_type3 b) {
	if (a.number > b.number)
		return (1);
	else
		return (0);
}

static bool lowsequence2(cloud_type2 a, cloud_type2 b) {
	if (a.index < b.index)
		return (1);
	else
		return (0);
}

static bool lowsequence3(cloud_type3 a, cloud_type3 b) {
	if (a.number_pattern < b.number_pattern)
		return (1);
	else
		return (0);
}

static int getnumber(char* pchLine) {
	int i = 0;
	int temp = 0;

	while (pchLine[i] != ' ')
		i++;

	i++;

	while ((pchLine[i] >= ZERO) && (pchLine[i] <= NINE)) {
		temp = temp * 10 + pchLine[i] - ZERO;
		i++;
	}

	return (temp);
}

static void getcloudandnumber(char* pchLine, int& cloud, int& number) {
	int i = 0;

	cloud = 0;

	while (pchLine[i] != ' ')
		i++;

	i++;

	while ((pchLine[i] >= ZERO) && (pchLine[i] <= NINE)) {
		cloud = cloud * 10 + pchLine[i] - ZERO;
		i++;
	}

	while (pchLine[i] != ' ')
		i++;

	i++;

	number = 0;

	while ((pchLine[i] >= ZERO) && (pchLine[i] <= NINE)) {
		number = number * 10 + pchLine[i] - ZERO;
		i++;
	}
}

static void swapcloud(cloud_type3& a, cloud_type3& b) {
	swap(a.number_pattern, b.number_pattern);
	swap(a.number, b.number);
	swap(a.cloud, b.cloud);
	swap(a.has_been_extended, b.has_been_extended);
}

static void q_sort(cloud_type3* repeats, int left, int right) {
	int pivot, l, r;
	l = left;
	r = right;
	pivot = repeats[(left + right) / 2].number;
	while (l < r) {
		while (repeats[l].number > pivot)
			++l;

		while (repeats[r].number < pivot)
			--r;

		if (l >= r)
			break;

		swapcloud(repeats[l], repeats[r]);
		++l;
		--r;
	}

	if (l == r)
		l++;

	if (left < r)
		q_sort(repeats, left, l - 1);
	if (l < right)
		q_sort(repeats, r + 1, right);
}

// transform the pattern sequence to the index of the array
// coding method: A=00, C=01, G= 10, T=11,
static void patterntoindex(const char *pchPattern, unsigned long& index,
		const int& patternsize) {
	int i;
	index = 0;
	for (i = 0; i < patternsize; i++) {
		index *= 4;
		if ((*(pchPattern + i) == 'a') || (*(pchPattern + i) == 'A'))
			index += 0;
		else if ((*(pchPattern + i) == 'c') || (*(pchPattern + i) == 'C'))
			index += 1;
		else if ((*(pchPattern + i) == 'g') || (*(pchPattern + i) == 'G'))
			index += 2;
		else if ((*(pchPattern + i) == 't') || (*(pchPattern + i) == 'T'))
			index += 3;
	}
}

static void number_pattern_to_kmer_sequence(char *pchPattern,
		const unsigned long& index, const int& patternsize) {
	int i;
	unsigned long value;
	unsigned long temp = index;

	for (i = patternsize - 1; i >= 0; i--) {
		value = temp % 4;
		if (value == 0)
			*(pchPattern + i) = 'A';
		else if (value == 1)
			*(pchPattern + i) = 'C';
		else if (value == 2)
			*(pchPattern + i) = 'G';
		else if (value == 3)
			*(pchPattern + i) = 'T';
		temp = temp / 4;
	}

	// This needs to be null terminated
	*(pchPattern + patternsize) = '\0';
}

ofstream testmers("testmers");

// Generates all the possible kmers (and their reverse complements) that are
// one substitution away from pchCore
static void getonesubstitutions(const char* core_kmer,
		unsigned long* array_of_testmer_number_patterns,
		int& number_of_testmers) {
	char alphabet[5] = "ACGT";

	int size = strlen(core_kmer);

	char* testmer = new char[size + 1];

	for (int i = 0; i < size; i++) {
		for (int k = 0; k < 4; k++) {
			if (alphabet[k] != core_kmer[i]) {
				strcpy(testmer, core_kmer);
				testmer[i] = alphabet[k];

				//STP: Added for reverse prototype
//				testmers << testmer << endl;

				patterntoindex(testmer,
						array_of_testmer_number_patterns[number_of_testmers],
						size);
				number_of_testmers++;
			}
		}
	}

	char* reverse_core_kmer = new char[size + 1];

	getreversecomplement(core_kmer, reverse_core_kmer);

	if (strcmp(reverse_core_kmer, core_kmer) != 0) {
		for (int i = 0; i < strlen(reverse_core_kmer); i++) {
			for (int k = 0; k < 4; k++) {
				if (alphabet[k] != reverse_core_kmer[i]) {
					strcpy(testmer, reverse_core_kmer);
					testmer[i] = alphabet[k];

					//STP: Added for reverse prototype
//					testmers << testmer << endl;

					patterntoindex(testmer,
							array_of_testmer_number_patterns[number_of_testmers],
							size);
					number_of_testmers++;
				}
			}
		}
	}

	delete[] (reverse_core_kmer);
	delete[] (testmer);
	/*free(reverse_core_kmer);
	 free(testmer);*/
}

static void gettwosubstitutions(const char* core_kmer,
		unsigned long* array_of_testmer_number_patterns,
		int& number_of_testmers) {
	char alphabet[5] = "ACGT";

	int size = strlen(core_kmer);

	char* testmer = (char*) malloc(sizeof(char) * (size + 1));

	for (int i = 0; i < size - 1; i++) {
		for (int j = i + 1; j < size; j++) {
			for (int k = 0; k < 4; k++) {
				for (int l = 0; l < 4; l++) {
					if ((alphabet[k] != core_kmer[i])
							&& (alphabet[l] != core_kmer[j])) {
						strcpy(testmer, core_kmer);
						testmer[i] = alphabet[k];
						testmer[j] = alphabet[l];

						//STP: Added for reverse prototype
//						testmers << testmer << endl;

						patterntoindex(testmer,
								array_of_testmer_number_patterns[number_of_testmers],
								size);
						number_of_testmers++;
					}
				}
			}
		}
	}

	char* reverse_core_kmer = (char*) malloc(sizeof(char) * (size + 1));

	getreversecomplement(core_kmer, reverse_core_kmer);

	for (int i = 0; i < strlen(reverse_core_kmer) - 1; i++) {
		for (int j = i + 1; j < strlen(reverse_core_kmer); j++) {
			for (int k = 0; k < 4; k++) {
				for (int l = 0; l < 4; l++) {
					if ((alphabet[k] != reverse_core_kmer[i])
							&& (alphabet[l] != reverse_core_kmer[j])) {
						strcpy(testmer, reverse_core_kmer);
						testmer[i] = alphabet[k];
						testmer[j] = alphabet[l];

						//STP: Added for reverse prototype
//						testmers << testmer << endl;

						patterntoindex(testmer,
								array_of_testmer_number_patterns[number_of_testmers],
								size);
						number_of_testmers++;
					}
				}
			}
		}
	}

	free(reverse_core_kmer);
	free(testmer);
}

static void getthreesubstitutions(char* core_kmer,
		unsigned long* array_of_testmer_number_patterns,
		int& number_of_testmers) {
	char alphabet[5] = "ACGT";

	int size = strlen(core_kmer);

	char* testmer = (char*) malloc(sizeof(char) * (size + 1));

	for (int i = 0; i < size - 2; i++) {
		for (int j = i + 1; j < size - 1; j++) {
			for (int m = j + 1; m < size; m++) {
				for (int k = 0; k < 4; k++) {
					for (int l = 0; l < 4; l++) {
						for (int n = 0; n < 4; n++) {
							if ((alphabet[k] != core_kmer[i])
									&& (alphabet[l] != core_kmer[j])
									&& ((alphabet[n] != core_kmer[m]))) {
								strcpy(testmer, core_kmer);
								testmer[i] = alphabet[k];
								testmer[j] = alphabet[l];
								testmer[m] = alphabet[n];

								//STP: Added for reverse prototype
//								testmers << testmer << endl;

								patterntoindex(testmer,
										array_of_testmer_number_patterns[number_of_testmers],
										size);
								number_of_testmers++;
							}
						}
					}
				}
			}
		}
	}

	char* reverse_core_kmer = (char*) malloc(sizeof(char) * (size + 1));

	getreversecomplement(core_kmer, reverse_core_kmer);

	for (int i = 0; i < size - 2; i++) {
		for (int j = i + 1; j < strlen(reverse_core_kmer) - 1; j++) {
			for (int m = j + 1; m < strlen(reverse_core_kmer); m++) {
				for (int k = 0; k < 4; k++) {
					for (int l = 0; l < 4; l++) {
						for (int n = 0; n < 4; n++) {
							if ((alphabet[k] != reverse_core_kmer[i])
									&& (alphabet[l] != reverse_core_kmer[j])
									&& ((alphabet[n] != reverse_core_kmer[m]))) {
								strcpy(testmer, reverse_core_kmer);
								testmer[i] = alphabet[k];
								testmer[j] = alphabet[l];
								testmer[m] = alphabet[n];

								//STP: Added for reverse prototype
//								testmers << testmer << endl;

								patterntoindex(testmer,
										array_of_testmer_number_patterns[number_of_testmers],
										size);
								number_of_testmers++;
							}
						}
					}
				}
			}
		}
	}

	free(reverse_core_kmer);
	free(testmer);
}

// specific to use 16mers, use some different kind of coding method for each 16mer
static void buildbitmatrix(bit_matrix& PatternMatrix, int* matrixsize,
		const int& patternsize) {
	cout << "Building bit matrix" << endl;
	exit(0);
	unsigned long index;

	char* pchPattern = (char*) malloc(sizeof(char) * (patternsize / 2 + 1));

	char* pchReverse = (char*) malloc(sizeof(char) * (patternsize / 2 + 1));

	// each line correspond to each pattern with index of the last 8 chars.
	for (int i = 0; i < pow(4, patternsize / 2); i++) {
		// index equal to i, get the last 8 characters of the pattern from index
		index = i;

		number_pattern_to_kmer_sequence(pchPattern, index, patternsize / 2);

		// get the reverse complement of the last 8 characters, then get the index of the reverse complement LR
		//the size of the vector[i] is LR+1;

		getreversecomplement(pchPattern, pchReverse);

		patterntoindex(pchReverse, index, patternsize / 2);

		PatternMatrix[i].reserve(index + 1);

		matrixsize[i] = index + 1;
	}

	free(pchPattern);
	free(pchReverse);

}

// specific to use 16mers, use some different kind of coding method for each 16mer
static void getpatternmatrixindex(char* pchPattern, unsigned long& i,
		unsigned long& j) {
	int size = strlen(pchPattern);

	char* pchTemp = (char*) malloc(sizeof(char) * (size / 2 + 1));

	// get the index of last 8 chars;
	strright(size / 2, pchPattern, pchTemp);

	patterntoindex(pchTemp, i, size / 2);

	// get the index of the first 8 chars;
	strleft(size / 2, pchPattern, pchTemp);

	patterntoindex(pchTemp, j, size / 2);

	free(pchTemp);
}

static bool isrepeatregion(vector<int>& occurrence, const int& percent) {
	vector<int>::iterator pOccurrence;

	int count = 0;

	for (pOccurrence = occurrence.begin(); pOccurrence < occurrence.end();
			pOccurrence++) {
		if (*(pOccurrence) > 0)
			count++;
	}

	if ((int) ((float) count / (float) (occurrence.size()) * 100) >= percent)
		return (1);
	else
		return (0);
}

// for 16mers calculation, oligos and its reverse complement are set to one
// read this kind of oligo count sets, get number1 and number2
static int readrepeatnumber(char* pchrepeatfile,
		int& number_of_oligos_above_end_threhold,
		int& number_of_oligos_above_copy_threhold, const int& oligo_size,
		const int& m_nStep1, const int& m_nEndthreshold,
		const int& copy_threshold) {

	FILE *pfRepeatfile;
	char *pchLine;
	char *pchTemp;

	number_of_oligos_above_end_threhold = 0;
	number_of_oligos_above_copy_threhold = 0;

	int end_threshold;

	if (m_nStep1 < m_nEndthreshold)
		end_threshold = m_nStep1;
	else
		end_threshold = m_nEndthreshold;

	if ((pfRepeatfile = fopen(pchrepeatfile, "r")) == NULL) {
		printf("Can not find the repeat file: %s.\n", pchrepeatfile);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);

		pchTemp = (char*) malloc(sizeof(char) * (oligo_size + 1));

		while (!feof(pfRepeatfile)) {
			if (fgets(pchLine, MAXLINECHAR, pfRepeatfile) != NULL) {
				strncpy(pchTemp, pchLine, oligo_size);
				*(pchTemp + oligo_size) = '\0';
				if (((isonessr(pchTemp) == 0) && (istwossr(pchTemp) == 0)
						&& (isthreessr(pchTemp) == 0)
						&& (isfourssr(pchTemp) == 0)) || keep_SSRs) {
					int oligo_count = getnumber(pchLine);
					//STP: Check if oligo count is above end end_threshold or copy threhold
					if (oligo_count >= end_threshold)
						number_of_oligos_above_end_threhold++;
					else if (oligo_count >= copy_threshold)
						number_of_oligos_above_copy_threhold++;
				}
			}
		}
		free(pchLine);
		free(pchTemp);
		fclose(pfRepeatfile);
	}
	return (1);
}

// read the oligos which is in mainclouds into repeats1 array
//STP: This does not take into account reverse complement of kmers.
static int readmainoligos(char *pchrepeatfile, cloud_type3* main_oligos,
		int& nOligos_above_end_threhold, const int& oligo_size,
		const int& m_nStep1, const int& m_nEndthreshold) {
	cout << "reading main oligos" << endl;
	FILE *pfRepeatfile;
	char *pchLine;
	char *pchTemp, *pchReverse;
	unsigned long index;

	nOligos_above_end_threhold = 0;

	int end_threshold;

	if (m_nStep1 < m_nEndthreshold)
		end_threshold = m_nStep1;
	else
		end_threshold = m_nEndthreshold;

	if ((pfRepeatfile = fopen(pchrepeatfile, "r")) == NULL) {
		printf("Can not find the repeat file: %s.\n", pchrepeatfile);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		pchTemp = (char*) malloc(sizeof(char) * (oligo_size + 1));

		while (!feof(pfRepeatfile)) {
			if (fgets(pchLine, MAXLINECHAR, pfRepeatfile) != NULL) {
				strncpy(pchTemp, pchLine, oligo_size);
				*(pchTemp + oligo_size) = '\0';

				// Commented out by STP to keep the short simple repeat
				if (((isonessr(pchTemp) == 0) && (istwossr(pchTemp) == 0)
						&& (isthreessr(pchTemp) == 0)
						&& (isfourssr(pchTemp) == 0)) || keep_SSRs) {
					int occur = getnumber(pchLine);

					if (occur >= end_threshold) {
						patterntoindex(pchTemp,
								main_oligos[nOligos_above_end_threhold].number_pattern,
								oligo_size);
						main_oligos[nOligos_above_end_threhold].number = occur;
						nOligos_above_end_threhold++;
					}
				}
			}
		}

		free(pchLine);
		free(pchTemp);
		fclose(pfRepeatfile);
	}

	return (1);
}

// build main clouds based on the algorithm
//STP: Which algorithm??
static void buildmainpcloud(cloud_type3* core_kmers, char* pchOutput,
		const int& number_of_core_kmers, const int& kmer_size,
		const int& core_threshold) {

	// Sort to descending order
	sort(core_kmers, core_kmers + number_of_core_kmers, highnumber3);

	unsigned long* core_kmer_number_patterns = (unsigned long*) malloc(
			sizeof(unsigned long) * number_of_core_kmers);

	for (int k = 0; k < number_of_core_kmers; k++)
		core_kmer_number_patterns[k] = core_kmers[k].number_pattern;

	//STP: How are we sorting these kmers?
	// Sort by number pattern (used to be called 'index')
	sort(core_kmers, core_kmers + number_of_core_kmers, lowsequence3);

	//STP: What does this do?
	int core_kmer_index_from_top = 0;
	// This finds the index for the core oligo with the index main_oligos_index[count]
	// This maps from index in main_oligos_index to index in main_oligos
	int core_kmer_index = sbsearch3(number_of_core_kmers, core_kmers,
			core_kmer_number_patterns[core_kmer_index_from_top]);

	char* seed_sequence = (char*) malloc(sizeof(char) * (kmer_size + 1));
	char* kmer_sequence = (char*) malloc(sizeof(char) * (kmer_size + 1));
	int *total_count_of_members_for_each_cloud = (int *) malloc(
			sizeof(int) * MAXCLOUD);
	char** core_kmer_sequences = (char**) malloc(
			sizeof(char) * MAXCLOUD * (kmer_size + 1));
	int *number_of_members_for_each_cloud = (int*) malloc(
			sizeof(int) * MAXCLOUD);
	int cloud_number_id = 0;

	int iRepeatThreesubstitution = 0;
	int iCoreThreesubstitution = 0;
	int result = 0;

	unsigned long *piRepeatThreesubstitution = (unsigned long*) malloc(
			2 * sizeof(unsigned long)
					* ((3 * kmer_size) + (9 * kmer_size * (kmer_size - 1) / 2)
							+ (27 * kmer_size * (kmer_size - 1)
									* (kmer_size - 2) / 2)));
	unsigned long *piCoreThreesubstitution = (unsigned long*) malloc(
			2 * sizeof(unsigned long)
					* ((3 * kmer_size) + (9 * kmer_size * (kmer_size - 1) / 2)
							+ (27 * kmer_size * (kmer_size - 1)
									* (kmer_size - 2) / 2)));

	//STP: For expansion network
	ofstream edges_out("edges_out");
	edges_out << "Source_core\tDestination_core" << endl;

	int total_number_of_clouds = 0;

	while ((core_kmer_index_from_top < number_of_core_kmers)
			&& (core_kmers[core_kmer_index].number >= core_threshold)) {
		// Cloud assignments must begin at 1 NOT 0
		// If the core kmer has not been assigned
		if (core_kmers[core_kmer_index].cloud == 0) {
			total_number_of_clouds++;
			core_kmers[core_kmer_index].cloud = total_number_of_clouds;
		}
		cloud_number_id = core_kmers[core_kmer_index].cloud;

		cout << "Cloud " << cloud_number_id << endl;
		number_pattern_to_kmer_sequence(kmer_sequence,
				core_kmers[core_kmer_index].number_pattern, kmer_size);

		cout << "Seed is " << kmer_sequence << endl;

		// get the core sequence of the pcloud
		core_kmer_sequences[cloud_number_id - 1] = new char[kmer_size + 1];

		number_pattern_to_kmer_sequence(
				core_kmer_sequences[cloud_number_id - 1],
				core_kmer_number_patterns[core_kmer_index_from_top], kmer_size);

		//cout << "Expanding around seed "
			//	<< core_kmer_sequences[cloud_number_id - 1] << endl;

		strcpy(seed_sequence, core_kmer_sequences[cloud_number_id - 1]);

		number_of_members_for_each_cloud[cloud_number_id - 1] = 1;
		total_count_of_members_for_each_cloud[cloud_number_id - 1] =
				core_kmers[core_kmer_index].number;

		core_kmers[core_kmer_index].has_been_extended = true;

		//STP: iCoreThreesubstitution is the number of patterns that are 3 subs
		// away from the seed kmer.
		iCoreThreesubstitution = 0;
		// Determine all the patterns within 3 substitutions
		// Put them in piCoreThreesubstitution
		getonesubstitutions(seed_sequence, piCoreThreesubstitution,
				iCoreThreesubstitution);
		gettwosubstitutions(seed_sequence, piCoreThreesubstitution,
				iCoreThreesubstitution);

		getthreesubstitutions(seed_sequence, piCoreThreesubstitution,
				iCoreThreesubstitution);
//		exit(1);
		//STP: For outputting all the edges between cores for a network
		// representation of the building method.

		// For all the testmers within 3 subs of the seedmer
		for (int i = 0; i < iCoreThreesubstitution; i++) {
			//STP: result is now the index in main_oligos for the pattern given
			// by the 'index' piCoreThreesubstitution[i] if found or -1 if not
			// found
			result = sbsearch3(number_of_core_kmers, core_kmers,
					piCoreThreesubstitution[i]);

			//If you find the pattern above in main_oligos
			if (result >= 0) {

				//If the main_oligo is not assigned, assign it
				// The unassigned value of .cloud must be 0
				// Cloud assignments must begin at 1 NOT 0
				if (core_kmers[result].cloud == 0) {
					core_kmers[result].cloud = cloud_number_id;
					number_pattern_to_kmer_sequence(kmer_sequence,
							core_kmers[result].number_pattern, kmer_size);
					//cout << "Adding " << kmer_sequence << endl;

					//STP: Added for expansion network
					edges_out << seed_sequence << "\t" << kmer_sequence << endl;

					number_of_members_for_each_cloud[cloud_number_id - 1]++;
					total_count_of_members_for_each_cloud[cloud_number_id - 1] +=
							core_kmers[result].number;
				}

				//NOTICE: how assignment and extension (expansion) are
				// completely independent
				// get another 1-sub, 2-sub and 3-sub with repeat above core
				// threshold
				// STP: I think 'extension' means has already been extended.
				if (core_kmers[result].has_been_extended == 0) {
					// STP: These lines are the same as above!
					core_kmers[result].has_been_extended = true;

					// Find all the oligos that are 3 or less distance
					iRepeatThreesubstitution = 0;
					number_pattern_to_kmer_sequence(kmer_sequence,
							core_kmers[result].number_pattern, kmer_size);

					//cout << "Expanding around core " << kmer_sequence << endl;

					getonesubstitutions(kmer_sequence,
							piRepeatThreesubstitution,
							iRepeatThreesubstitution);

					gettwosubstitutions(kmer_sequence,
							piRepeatThreesubstitution,
							iRepeatThreesubstitution);

					getthreesubstitutions(kmer_sequence,
							piRepeatThreesubstitution,
							iRepeatThreesubstitution);

					//STP: For expansion network
					string core_sequence(kmer_sequence);

					// Do the second round of expansion
					for (int j = 0; j < iRepeatThreesubstitution; j++) {
						result = sbsearch3(number_of_core_kmers, core_kmers,
								piRepeatThreesubstitution[j]);

						if (result >= 0) {
							if (core_kmers[result].cloud == 0) {
								core_kmers[result].cloud = cloud_number_id;
								number_pattern_to_kmer_sequence(kmer_sequence,
										core_kmers[result].number_pattern,
										kmer_size);
								//cout << "Adding " << kmer_sequence << endl;

								//STP: For making the expansion network
								edges_out << core_sequence << "\t"
										<< kmer_sequence << endl;

								//edges_out << distanceBetween(core_sequence, kmer_sequence);

								number_of_members_for_each_cloud[cloud_number_id
										- 1]++;
								total_count_of_members_for_each_cloud[cloud_number_id
										- 1] += core_kmers[result].number;
							}
						}
					}
					//cout << "Done expanding around core " << core_sequence
						//	<< endl;
				}
			}
		}

		//cout << "Done expanding around seed "
			//	<< core_kmer_sequences[cloud_number_id - 1] << endl;

		// move to next core repeats
		// Keep counting up until you find a main oligo that has not been
		// assigned
		// If this line is changed to checking if the main oligo has not been
		// /extended/ then the method would continue to expand the cloud after
		// the second expansion. Notice how we have not expanded around the
		// core kmers that are 2 steps from a seedmer.
		// Keep counting if
		while (
				// We haven't seen the last core. Stop if we have.
				(core_kmer_index_from_top < number_of_core_kmers)
				and
				(
				// EITHER
				// We are not expanding recursively and the core we are
				// looking at has been assigned already. Stop if it has not.
				(not expand_recursively and core_kmers[core_kmer_index].cloud != 0)
				// OR
				or
				// We are expanding recursively and the core we are looking at
				// has been assigned AND extended. Stop if it has not.
				(expand_recursively and core_kmers[core_kmer_index].cloud != 0
						and core_kmers[core_kmer_index].has_been_extended != 0))
				) {
			 core_kmer_index_from_top++;

			// This is doing extra work. It should be outside this while loop.
			if (core_kmer_index_from_top <= number_of_core_kmers - 1)
				core_kmer_index = sbsearch3(number_of_core_kmers, core_kmers,
						core_kmer_number_patterns[core_kmer_index_from_top]);

		}
	}

	cout << "total clouds formed is " << total_number_of_clouds << endl;
	free(piRepeatThreesubstitution);
	free(piCoreThreesubstitution);
	//output the main clouds information
	FILE *pfOutput = fopen(pchOutput, "wb");

	for (int k = 0; k < total_number_of_clouds; k++)
		fprintf(pfOutput, "%d\t%d\t%d\t%s\n", k + 1,
				number_of_members_for_each_cloud[k],
				total_count_of_members_for_each_cloud[k],
				core_kmer_sequences[k]);

	free(core_kmer_number_patterns);
	free(kmer_sequence);
	free(seed_sequence);
	fclose(pfOutput);

	for (int k = 0; k < total_number_of_clouds; k++) {
		delete[] (core_kmer_sequences[k]);
	}

	free(core_kmer_sequences);
	free(total_count_of_members_for_each_cloud);
	free(number_of_members_for_each_cloud);

}

//output the mainclouds assignments of each oligo in main clouds
static void outputmainclouds(cloud_type3* main_oligos, const char* pchResult,
		const int& nOligos_above_end_threshold, const int& size) {
	FILE *pfResult = fopen(pchResult, "wb");

	char *pchPattern = (char*) malloc(sizeof(char) * (size + 1));

	sort(main_oligos, main_oligos + nOligos_above_end_threshold, highnumber3);

	for (int i = 0; i < nOligos_above_end_threshold; i++) {
		number_pattern_to_kmer_sequence(pchPattern,
				main_oligos[i].number_pattern, size);
		if (main_oligos[i].cloud > 9000)
			std::cout << "Cloud id is over 9000" << endl;
		fprintf(pfResult, "%s %d %d\n", pchPattern, main_oligos[i].cloud,
				main_oligos[i].number);
	}

	fclose(pfResult);
	free(pchPattern);
}

// read main clouds assignments information into three different sets
static int readmainclouds(const char* pchMainCloudassign, cloud_type2* pCloudsA,
		int& numberA, cloud_type2* pCloudsB, int& numberB,
		cloud_type2* pCloudsC, int& numberC, const int& size,
		const int& primary_threshold, const int& secondary_threshold, const
		int& tertiary_threshold) {
	FILE *pfCloudassign;
	char *pchLine;
	char *pchTemp;

	numberA = 0;
	numberB = 0;
	numberC = 0;

	int count;
	int cloud;
	unsigned long index;

	if ((pfCloudassign = fopen(pchMainCloudassign, "r")) == NULL) {
		printf("Can not find the cloud assign file: %s.\n", pchMainCloudassign);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		pchTemp = (char*) malloc(sizeof(char) * (size + 1));
		while (!feof(pfCloudassign)) {
			if (fgets(pchLine, MAXLINECHAR, pfCloudassign) != NULL) {
				getcloudandnumber(pchLine, cloud, count);

				if (count >= tertiary_threshold) {
					pCloudsA[numberA].cloud = cloud;
					strncpy(pchTemp, pchLine, size);
					*(pchTemp + size) = '\0';
					patterntoindex(pchTemp, index, size);
					pCloudsA[numberA].index = index;
					numberA++;
				} else if (count >= secondary_threshold) {
					pCloudsB[numberB].cloud = cloud;
					strncpy(pchTemp, pchLine, size);
					*(pchTemp + size) = '\0';
					patterntoindex(pchTemp, index, size);
					pCloudsB[numberB].index = index;
					numberB++;
				} else if (count >= primary_threshold) {
					pCloudsC[numberC].cloud = cloud;
					strncpy(pchTemp, pchLine, size);
					*(pchTemp + size) = '\0';
					patterntoindex(pchTemp, index, size);
					pCloudsC[numberC].index = index;
					numberC++;
				}
			}
		}
		free(pchLine);
		free(pchTemp);
		fclose(pfCloudassign);
	}

	sort(pCloudsA, pCloudsA + numberA, lowsequence2);
	sort(pCloudsB, pCloudsB + numberB, lowsequence2);
	sort(pCloudsC, pCloudsC + numberC, lowsequence2);

	return (1);
}

// assign the oligos in accessory regions into P clouds constructed in former step
static int buildaccessarypcloud(char *pchrepeatfile, char* pchOutput,
		cloud_type2* core_kmers_above_tertiary,
		const int& number_of_cores_above_tertiary,
		cloud_type2* core_kmers_above_secondary,
		const int& number_of_cores_above_secondary,
		cloud_type2* core_kmers_above_primary,
		const int& number_of_cores_above_primary, int& number2,
		const int& oligo_size, const int& m_nStep1, const int& core_threshold,
		const int& outer_threshold) {

	//STP: pCloudsA, B, and C hold core oligos with counts above the tertiary,
	// secondary, and primary thresholds respectively. This determines the
	// expansion distance around each core oligo.
	FILE *pfRepeatfile;
	char *pchLine;
	char *core_kmer, *pchReverse, *testmer;

	//STP: The sizes of these are the number of possible kmers with 1, 2, and 3,
	// substitutions
	// sum ( 3^n * size * (3(size - n))! / n!) for n = {1, 2, 3}
	unsigned long *piRepeatOnesubstitution = (unsigned long*) malloc(
			2 * sizeof(unsigned long) * (3 * oligo_size));

	unsigned long *piRepeatTwosubstitution = (unsigned long*) malloc(
			2 * sizeof(unsigned long)
					* ((3 * oligo_size)
							+ (9 * oligo_size * (oligo_size - 1) / 2)));

	unsigned long *piRepeatThreesubstitution = (unsigned long*) malloc(
			2 * sizeof(unsigned long)
					* ((3 * oligo_size)
							+ (9 * oligo_size * (oligo_size - 1) / 2)
							+ (27 * oligo_size * (oligo_size - 1)
									* (oligo_size - 2) / (3 * 2))));
	int iRepeatOnesubstitution;
	int iRepeatTwosubstitution;
	int iRepeatThreesubstitution;
	int number;

	number2 = 0;

	int threshold;

	if (m_nStep1 < core_threshold)
		threshold = m_nStep1;
	else
		threshold = core_threshold;

	int copynumber;
	int result;
	int cloudindex;

	FILE* pfOutput = fopen(pchOutput, "wb");

	//STP: For expansion network
	ofstream edges_out("edges_out", fstream::app);

	if ((pfRepeatfile = fopen(pchrepeatfile, "r")) == NULL) {
		printf("Can not find the repeat file: %s.\n", pchrepeatfile);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		core_kmer = (char*) malloc(sizeof(char) * (oligo_size + 1));
		testmer = (char*) malloc(sizeof(char) * (oligo_size + 1));

		while (!feof(pfRepeatfile)) {
			number = 0;

			cloud_type2* repeatacc = (cloud_type2*) malloc(
					sizeof(cloud_type2) * MAXACCMER);
			// read in a chunk of accessory repeats
			while ((!feof(pfRepeatfile)) && (number < MAXACCMER)) {
				if (fgets(pchLine, MAXLINECHAR, pfRepeatfile) != NULL) {
					strncpy(core_kmer, pchLine, oligo_size);
					*(core_kmer + oligo_size) = '\0';

					if (((isonessr(core_kmer) == 0)
							&& (istwossr(core_kmer) == 0)
							&& (isthreessr(core_kmer) == 0)
							&& (isfourssr(core_kmer) == 0)) || keep_SSRs) {
						int occur = getnumber(pchLine);

						if ((occur < threshold) && (occur >= outer_threshold)) {
							// insert the acc into repeats[number]
							patterntoindex(core_kmer, repeatacc[number].index,
									oligo_size);

							number++;
						}
					}
				}
			}

			sort(repeatacc, repeatacc + number, lowsequence2);

			//STP: Sort the core kmers before expanding
			//STP: This is not possible because the counts are not included in
			// this type of cloud.
//			sort(core_kmers_above_tertiary, core_kmers_above_tertiary + number_of_cores_above_tertiary, highnumber2);
//			sort(core_kmers_above_secondary, core_kmers_above_secondary+ number_of_cores_above_secondary, highnumber3);
//			sort(core_kmers_above_primary, core_kmers_above_primary + number_of_cores_above_primary, highnumber3);

			// build the accessory p clouds for these repeats chunk
			for (int i = 0; i < number_of_cores_above_tertiary; i++) {
				number_pattern_to_kmer_sequence(core_kmer,
						core_kmers_above_tertiary[i].index, oligo_size);

				// get the 3-mutation sets of core_kmer;
				iRepeatThreesubstitution = 0;

				getthreesubstitutions(core_kmer, piRepeatThreesubstitution,
						iRepeatThreesubstitution);
				gettwosubstitutions(core_kmer, piRepeatThreesubstitution,
						iRepeatThreesubstitution);
				getonesubstitutions(core_kmer, piRepeatThreesubstitution,
						iRepeatThreesubstitution);

				for (int count = 0; count < iRepeatThreesubstitution; count++) {
					result = sbsearch2(number, repeatacc,
							piRepeatThreesubstitution[count]);

					if (result >= 0) {
						if (repeatacc[result].cloud == 0) {
							repeatacc[result].cloud =
									core_kmers_above_tertiary[i].cloud;

							//STP: For making the expansion network
							number_pattern_to_kmer_sequence(testmer,
									piRepeatThreesubstitution[count],
									oligo_size);
							edges_out << core_kmer << "\t" << testmer << endl;
						}
					}
				}
			}

			// NOTICE: the piRepeatTwosubstitutions are not cleared between
			// rounds. This is OK because we keep track of how many repeat
			// two substitutions we have in iRepeatTwosubstitution.
			for (int i = 0; i < number_of_cores_above_secondary; i++) {
				number_pattern_to_kmer_sequence(core_kmer,
						core_kmers_above_secondary[i].index, oligo_size);

				// get the 2-mutation sets of core_kmer;
				iRepeatTwosubstitution = 0;

				gettwosubstitutions(core_kmer, piRepeatTwosubstitution,
						iRepeatTwosubstitution);
				getonesubstitutions(core_kmer, piRepeatTwosubstitution,
						iRepeatTwosubstitution);

				for (int count = 0; count < iRepeatTwosubstitution; count++) {
					result = sbsearch2(number, repeatacc,
							piRepeatTwosubstitution[count]);

					if (result >= 0) {
						if (repeatacc[result].cloud == 0) {
							repeatacc[result].cloud =
									core_kmers_above_secondary[i].cloud;

							//STP: For making the expansion network
							number_pattern_to_kmer_sequence(testmer,
									piRepeatTwosubstitution[count], oligo_size);
							edges_out << core_kmer << "\t" << testmer << endl;
						}
					}
				}
			}

			for (int i = 0; i < number_of_cores_above_primary; i++) {
				number_pattern_to_kmer_sequence(core_kmer,
						core_kmers_above_primary[i].index, oligo_size);

				// get the 1-mutation sets of core_kmer;
				iRepeatOnesubstitution = 0;

				getonesubstitutions(core_kmer, piRepeatOnesubstitution,
						iRepeatOnesubstitution);

				for (int count = 0; count < iRepeatOnesubstitution; count++) {
					result = sbsearch2(number, repeatacc,
							piRepeatOnesubstitution[count]);

					if (result >= 0) {
						if (repeatacc[result].cloud == 0) {
							repeatacc[result].cloud =
									core_kmers_above_primary[i].cloud;

							//STP: For making the expansion network
							number_pattern_to_kmer_sequence(testmer,
									piRepeatOnesubstitution[count], oligo_size);
							edges_out << core_kmer << "\t" << testmer << endl;
						}
					}
				}
			}

			//output the accessory p clouds information for these repeats
			for (int i = 0; i < number; i++) {
				if (repeatacc[i].cloud != 0) {
					number_pattern_to_kmer_sequence(core_kmer,
							repeatacc[i].index, oligo_size);
					fprintf(pfOutput, "%s %d\n", core_kmer, repeatacc[i].cloud);
				}
			}

			free(repeatacc);
		}

		free(pchLine);
		free(core_kmer);
		free(testmer);
		fclose(pfRepeatfile);
		fclose(pfOutput);

	}
	free(piRepeatOnesubstitution);
	free(piRepeatTwosubstitution);
	free(piRepeatThreesubstitution);

	return (1);
}

//for 16mers, build the bool patternmatrix to ascertain which oligo is in the P clouds
static int readclouds(const char* pchMainCloudassign,
		const char* pchAccCloudassign, bit_matrix& PatternMatrix,
		int* matrixsize, const int& size) {
	FILE *pfMainCloudassign;
	FILE *pfAccCloudassign;
	char *pchLine;
	char *pchTemp;
	char* pchReverse;
	unsigned long left, right;

	if ((pfMainCloudassign = fopen(pchMainCloudassign, "r")) == NULL) {
		printf("Can not find the cloud assign file: %s.\n", pchMainCloudassign);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		pchTemp = (char*) malloc(sizeof(char) * (size + 1));
		pchReverse = (char*) malloc(sizeof(char) * (size + 1));

		while (fgets(pchLine, MAXLINECHAR, pfMainCloudassign) != NULL) {
			if (strlen(pchLine) < size) {
				printf("String not long enough %s\n", pchLine);
			}
			strncpy(pchTemp, pchLine, size);
			*(pchTemp + size) = '\0';
			getpatternmatrixindex(pchTemp, right, left);

			if (left >= matrixsize[right]) {
				getreversecomplement(pchTemp, pchReverse);
				strcpy(pchTemp, pchReverse);
				getpatternmatrixindex(pchTemp, right, left);

			}
			PatternMatrix[right][left] = true;

		}
		free(pchLine);
		free(pchTemp);
		free(pchReverse);
		fclose(pfMainCloudassign);
	}

	if ((pfAccCloudassign = fopen(pchAccCloudassign, "r")) == NULL) {
		printf("Can not find the cloud assign file: %s.\n", pchAccCloudassign);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		pchTemp = (char*) malloc(sizeof(char) * (size + 1));
		pchReverse = (char*) malloc(sizeof(char) * (size + 1));

		while (fgets(pchLine, MAXLINECHAR, pfAccCloudassign) != NULL) {
			strncpy(pchTemp, pchLine, size);
			*(pchTemp + size) = '\0';

			getpatternmatrixindex(pchTemp, right, left);

			if (left >= matrixsize[right]) {
				getreversecomplement(pchTemp, pchReverse);
				strcpy(pchTemp, pchReverse);
				getpatternmatrixindex(pchTemp, right, left);
			}

			PatternMatrix[right][left] = true;

		}
		free(pchLine);
		free(pchTemp);
		free(pchReverse);
		fclose(pfAccCloudassign);
	}

	return (1);
}

//for 16mers, annotate the genome based upon whether it is in the p clouds, and get the repeat regions
//but no p cloud pattern included in that annotation.
static int GenomeScanAndIdentify(const char* pchGenome, const char* pchOutfile,
		const char* pchRegionfile, const bit_matrix& PatternMatrix,
		const int* matrixsize, const int& size, const int& windowsize,
		const int& percent, const int& m_nChunksize,
		const unsigned int& m_nGenomesize) {
	FILE *pfGenome;

	char *pchPattern;
	int *piReadCount = (int*) malloc(sizeof(int));
	char *pchSequence = (char*) malloc(sizeof(char) * m_nChunksize);
	pchPattern = (char *) malloc(sizeof(char) * (size + 1));
	char *pchReverse = (char *) malloc(sizeof(char) * (size + 1));
	long long iOffset = 0;
	int iRead;

	FILE *pfOut;

	pfOut = fopen(pchOutfile, "wb");

	int patternnumber;
	unsigned long index;
	unsigned long left;
	unsigned long right;

	FILE *pfRegion;

	vector<int> occurrence;

	long long icount = 0;
	long long iStart = 0, iEnd = 0;
	long long iFormerStart = 0, iFormerEnd = 0;
	long long totalsize = 0;

	pfRegion = fopen(pchRegionfile, "wb");

	if ((pfGenome = fopen(pchGenome, "r")) == NULL) {
		printf("Can not find the genome file: %s.\n", pchGenome);
		return (0);
	} else {
		do {
			if (NULL == pchSequence) {
				printf("The system can not allocate the memory!\n");
				free(pchPattern);
				free(pchReverse);
				free(piReadCount);
				return (0);
			}

			if (iOffset + m_nChunksize - 1 <= m_nGenomesize)
				iRead = readfromfile(pfGenome, pchSequence, iOffset,
						m_nChunksize, piReadCount);
			else
				iRead = readfromfile(pfGenome, pchSequence, iOffset,
						m_nGenomesize - iOffset, piReadCount);

			if (iRead == 0) {

				free(pchPattern);
				free(pchSequence);
				free(pchReverse);
				free(piReadCount);
				return (0);
			} else {
				for (int count = 0; count <= *piReadCount - size; count++) {
					if ((count % 50 == 0) && (count > 0))
						fprintf(pfOut, "\n");

					getsubstring(pchSequence, pchPattern, count,
							count + size - 1);
					pchPattern[size] = '\0';
					patternnumber = 0;

					if (issegmentvalid(pchPattern) == 1) {
						getpatternmatrixindex(pchPattern, right, left);

						if (left >= matrixsize[right]) {
							getreversecomplement(pchPattern, pchReverse);
							strcpy(pchPattern, pchReverse);
							getpatternmatrixindex(pchPattern, right, left);
						}

						if (PatternMatrix[right][left])
							patternnumber = 1;
					}

					//output the p cloud annotation file;
					fprintf(pfOut, "%d ", patternnumber);

					//get the annotated repeat region
					icount++;

					if (icount <= windowsize) {
						occurrence.push_back(patternnumber);

						if (icount == windowsize) {
							if (isrepeatregion(occurrence, percent)) {
								iStart = icount - windowsize + 1;
								iFormerStart = iStart;
								iEnd = 0;
							}
						}
					} else {
						occurrence.erase(occurrence.begin());
						occurrence.push_back(patternnumber);

						if (isrepeatregion(occurrence, percent)) {
							if (iStart == 0) {
								iStart = icount - windowsize + 1;

								if (iStart > iFormerEnd + 1) {
									if ((iFormerEnd != 0)
											&& (iFormerStart != 0)) {
										fprintf(pfRegion, "%lld %lld\n",
												iFormerStart, iFormerEnd);
										totalsize += iFormerEnd - iFormerStart
												+ 1;
									}

									iFormerStart = iStart;
								}

								iEnd = 0;
							}
						} else {
							if ((iStart != 0) && (iEnd == 0)) {
								int i = windowsize - 1;

								while (occurrence[i] == 0)
									i--;

								iEnd = icount + size - 2 - (windowsize - 1 - i);
								iFormerEnd = iEnd;
								iStart = 0;
							}
						}
					}
				}
			}
			iOffset += m_nChunksize - size + 1;
		} while ((iOffset < m_nGenomesize - size + 1) && (iRead == 2));

		fclose(pfGenome);
	}

	fprintf(pfRegion, "%lld %lld\n", iFormerStart, iFormerEnd);

	totalsize += iFormerEnd - iFormerStart + 1;

	fprintf(pfRegion, "%lld", totalsize);
	fflush(pfRegion);
	fclose(pfRegion);
	fclose(pfOut);
	free(pchPattern);
	free(pchSequence);
	free(piReadCount);
	free(pchReverse);

	return (1);
}

//for 16mers, build the bool patternmatrix to ascertain which oligo is in the P clouds
//STP: Modified to annotate the positions of the clouds in repeat regions
static int readclouds1(const char* pchMainCloudassign,
		const char* pchAccCloudassign, bitvector& PatternVector,
		vector<int>& CloudIdVector, const int& size) {
	FILE *pfMainCloudassign;
	FILE *pfAccCloudassign;
	char *pchLine;
	char *pchTemp;
	char* pchReverse;
	unsigned long index;

	if ((pfMainCloudassign = fopen(pchMainCloudassign, "r")) == NULL) {
		printf("Can not find the cloud assign file: %s.\n", pchMainCloudassign);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		pchTemp = (char*) malloc(sizeof(char) * (size + 1));
		pchReverse = (char*) malloc(sizeof(char) * (size + 1));

		while (!feof(pfMainCloudassign)) {
			if (fgets(pchLine, MAXLINECHAR, pfMainCloudassign) != NULL) {
				strncpy(pchTemp, pchLine, size);
				*(pchTemp + size) = '\0';

				patterntoindex(pchTemp, index, size);

				cout << "Found kmer " << pchTemp << " with index " << index
						<< endl;

				PatternVector[index] = true;
				CloudIdVector[index] = getnumber(pchLine);

				getreversecomplement(pchTemp, pchReverse);
				patterntoindex(pchReverse, index, size);

				cout << "Found kmer " << pchTemp << " with index " << index
						<< endl;

				PatternVector[index] = true;
				CloudIdVector[index] = getnumber(pchLine);
			}
		}

		free(pchLine);
		free(pchTemp);
		free(pchReverse);
		fclose(pfMainCloudassign);
	}

	if ((pfAccCloudassign = fopen(pchAccCloudassign, "r")) == NULL) {
		printf("Can not find the cloud assign file: %s.\n", pchAccCloudassign);
		return (0);
	} else {
		pchLine = (char *) malloc(sizeof(char) * MAXLINECHAR);
		pchTemp = (char*) malloc(sizeof(char) * (size + 1));
		pchReverse = (char*) malloc(sizeof(char) * (size + 1));

		while (!feof(pfAccCloudassign)) {
			if (fgets(pchLine, MAXLINECHAR, pfAccCloudassign) != NULL) {
				strncpy(pchTemp, pchLine, size);
				*(pchTemp + size) = '\0';

				patterntoindex(pchTemp, index, size);
				PatternVector[index] = true;

				//STP: Added for printing the positions of the clouds
				// in a repeat region
				CloudIdVector[index] = getnumber(pchLine);
				cout << "Found kmer " << pchTemp << " with index " << index
						<< endl;

				getreversecomplement(pchTemp, pchReverse);
				patterntoindex(pchReverse, index, size);

				PatternVector[index] = true;

				//STP: Added for printing the positions of the clouds
				// in a repeat region
				CloudIdVector[index] = getnumber(pchLine);
				cout << "Found kmer " << pchTemp << " with index " << index
						<< endl;
			}
		}

		free(pchLine);
		free(pchTemp);
		free(pchReverse);
		fclose(pfAccCloudassign);
	}

	return (1);
}

//for NON 16mers, annotate the genome based upon whether it is in the p clouds, and get the repeat regions
//but no p cloud pattern included in that annotation.
static int GenomeScanAndIdentify1(const char* pchGenome, const char* pchOutfile,
		const char* pchRegionfile, const bitvector& PatternVector,
		const vector<int>& CloudIdVector, const int& kmer_size,
		const int& windowsize, const int& percent, const int& m_nChunksize,
		const unsigned int& m_nGenomesize) {

	cout << "Scanning genome for clouds..." << endl;
	FILE *pfGenome;

	int *actual_chunk_size = (int*) malloc(sizeof(int));
	char *chunk = (char*) malloc(sizeof(char) * m_nChunksize);
	char *kmer_sequence = (char *) malloc(sizeof(char) * (kmer_size + 1));
	char *pchReverse = (char *) malloc(sizeof(char) * (kmer_size + 1));

	long long chunk_start = 0;
	int iRead;

	FILE *pfOut = fopen(pchOutfile, "wb");

	int patternnumber = 0;
	int cloud_id = 0;
	unsigned long index = 0;

	FILE *pfRegion = fopen(pchRegionfile, "wb");

	vector<int> occurrence(windowsize, 0);

	//STP: Added for annotating which clouds are in which repeat regions
	// Since queue has no 'erase' function, I'll use a deque which is the
	// implementation anyway
	deque<int> cloud_ids_in_region;

	long long genome_position = 0;
	long long start = 0, end = 0;
	long long former_start = 0;
	long long former_end = -1; // This must be initialized to < 0 in case the
								// first window is repetitive
	long long totalsize = 0;

	bool previous_window_was_repetitive = false;
	bool have_annotated_first_region = false;

	//STP: Added to print a header
	if (print_clouds_in_regions) {
		fputs("Start\tEnd\tClouds\n", pfRegion);
	}
	else {
		fputs("Start\tEnd\n", pfRegion);
	}



	if ((pfGenome = fopen(pchGenome, "r")) == NULL) {
		printf("Can not find the genome file: %s.\n", pchGenome);
		return (0);
	} else {
		//STP: The preprocessed fasta file has ">Processed reads" in the first line
		// This removes that header so the regions actually correspond to
		// the regions in the genome
		if (not print_legacy_regions) {
			chunk_start = 18;
		}
		do {
			if (NULL == chunk) {
				printf("The system can not allocate the memory!\n");
				free(kmer_sequence);
				free(pchReverse);
				free(actual_chunk_size);
				return (0);
			}

			/* Does this handle the stretch of sequence at the junction
			 between reads? Those need to be handled specially.
			 They are handled specially below when the new iOffset is
			 calculated. */
			// STP: all ends should be exclusive -- double check
			int chunk_end = chunk_start + m_nChunksize - 1;
			if (chunk_end <= m_nGenomesize)
				iRead = readfromfile(pfGenome, chunk, chunk_start, m_nChunksize,
						actual_chunk_size);
			else
				iRead = readfromfile(pfGenome, chunk, chunk_start,
						m_nGenomesize - chunk_start, actual_chunk_size);

			if (iRead == 0) {
				free(kmer_sequence);
				free(chunk);
				free(pchReverse);
				free(actual_chunk_size);
				return (0);
			} else {
				//STP: Read has succeeded
				for (int site = 0; site <= *actual_chunk_size - kmer_size;
						site++, genome_position++) {

					if ((site % 80 == 0) && (site != 0))
						fprintf(pfOut, "\n");

					//STP: We don't need to copy the sequence here.
					// We could simply have a pointer to the chunk
					getsubstring(chunk, kmer_sequence, site,
							site + kmer_size - 1);

					kmer_sequence[kmer_size] = '\0';

					patternnumber = 0;
					cloud_id = 0;

					if (issegmentvalid(kmer_sequence)) {
						patterntoindex(kmer_sequence, index, kmer_size);

						if (PatternVector[index]) {
							patternnumber = 1;

							cloud_id = CloudIdVector[index];
						}
					}

					//output the p cloud annotation file;
					fprintf(pfOut, "%d", patternnumber);

					occurrence.erase(occurrence.begin());
					occurrence.push_back(patternnumber);

					cloud_ids_in_region.push_back(cloud_id);

					if (genome_position < (windowsize - 1)) {
						// for example, when genome_position is 9, we have
						// looked at 10 kmers and filled a window of 10
						continue;
						// Don't even consider if it is repetitive or not
					}

					if (isrepeatregion(occurrence, percent)) {
						if (not previous_window_was_repetitive) {
							if (print_legacy_regions) {
								start = genome_position - (windowsize - 1) + 1;
							}
							else {
								start = genome_position - (windowsize - 1);
							}

							if (start > former_end) {
								// If the next repeat starts past the end
								// of the former repeat
								// If not, merge.
								// I'm not sure this if statement works with
								// the legacy regions.

								if (have_annotated_first_region) {
									// Since we print the previous region,
									// we need to have seen the first region
									// before we print
									fprintf(pfRegion, "%lld\t%lld\t",
											former_start, former_end);

									if (print_clouds_in_regions) {
										int number_of_clouds_to_print = former_end
												- kmer_size - former_start + 1;

										for (int cloud = 0;
												cloud < number_of_clouds_to_print;
												cloud++) {
											fprintf(pfRegion, "%i ",
													cloud_ids_in_region.front());
											cloud_ids_in_region.pop_front();
										}
									}
									fputs("\n", pfRegion);

									// Erase all clouds up to a window size before the end
									cloud_ids_in_region.erase(
											cloud_ids_in_region.begin(),
											cloud_ids_in_region.end()
													- windowsize);
								}
								else {
									have_annotated_first_region = true;
								}

								former_start = start;
							}
						}
						previous_window_was_repetitive = true;
					} else {
						// Current window is not repetitive
						if (previous_window_was_repetitive) {
							if (print_legacy_regions) {
								// find the position of the  last cloud in the
								// previous repeat region
								// then add kmer_size to that

								int i = windowsize - 1;

								while (occurrence[i] == 0)
									i--;
								// The legacy end is not exclusive
								former_end = genome_position + kmer_size - 1 - (windowsize - 1 - i);
							}
							else {
								/*
								 * The new way to calculate the former end is
								 * more true to the method published in the
								 * pclouds paper by wanjun.
								 */
								former_end = genome_position - 1 + kmer_size; // ends are exclusive
							}
						} else {
							if (not have_annotated_first_region) {
								cloud_ids_in_region.pop_front();
							}
						}
						previous_window_was_repetitive = false;
					}
				}
			}
			// Subtract size in order to handle the break between chunks
			// properly
			chunk_start += m_nChunksize - kmer_size + 1;
		} while ((chunk_start < m_nGenomesize - kmer_size + 1) && (iRead == 2));

		fclose(pfGenome);
	}

	// If the last window was repetitive, update the former end
	// This essentially makes the former end to be the genome size
	if (previous_window_was_repetitive) {
		if (print_legacy_regions) {
			// What should happen here? '
			// The old Pclouds did not annotate the final region correctly.
			// Should I fix that here?
			// For now, I have fixed it...

			// find the position of the  last cloud in the
			// previous repeat region
			// then add kmer_size to that

			int i = windowsize - 1;

			while (occurrence[i] == 0)
				i--;
			// The legacy end is not exclusive
			former_end = genome_position + kmer_size - 1 - (windowsize - 1 - i);
		}
		else {
			/*
			 * The new way to calculate the former end is
			 * more true to the method published in the
			 * pclouds paper by wanjun.
			 */
			former_end = genome_position - 1 + kmer_size; // ends are exclusive
		}
	}

	if (have_annotated_first_region) {
		fprintf(pfRegion, "%lld\t%lld\t", former_start, former_end);
		if (print_clouds_in_regions) {
			int number_of_clouds_to_print = former_end - kmer_size - former_start
					+ 1;

			for (int cloud = 0; cloud < number_of_clouds_to_print; cloud++) {
				fprintf(pfRegion, "%i ", cloud_ids_in_region.front());
				cloud_ids_in_region.pop_front();
			}
		}
		fputs("\n", pfRegion);
	}

	fflush(pfRegion);
	fclose(pfRegion);
	fclose(pfOut);
	free(kmer_sequence);
	free(chunk);
	free(actual_chunk_size);
	free(pchReverse);

	return (1);
}

int pcloudsdissection(const char* pchControlfile) {
	int oligo_size, windowsize, percent, m_nGetclouds, m_nDissection;

	int m_nStep1, m_nStep2, m_nStep3, core_threshold, outer_threshold,
			m_nChunksize;
	unsigned int m_nGenomesize;

	char pchrepeatfile[MAXFILENAMELENGTH], pchMainClouds[MAXFILENAMELENGTH],
			pchMainAssign[MAXFILENAMELENGTH], pchAccAssign[MAXFILENAMELENGTH];
	char pchGenome[MAXFILENAMELENGTH], pchAnnotationfile[MAXFILENAMELENGTH],
			pchRegionfile[MAXFILENAMELENGTH];

	int number_of_core_kmers, number_of_outer_kmers;

	// read the control file the get the parameters of the program
	if (!ReadPcloudsControlfile(pchControlfile, oligo_size, outer_threshold,
			core_threshold, m_nStep1, m_nStep2, m_nStep3, m_nChunksize,
			m_nGenomesize, windowsize, percent, m_nGetclouds, m_nDissection,
			pchrepeatfile, pchGenome, pchMainClouds, pchMainAssign,
			pchAccAssign, pchAnnotationfile, pchRegionfile)) {
		cerr << "Open Control file error." << endl;
		return (-1);
	}

	if (m_nGetclouds) {
		if (readrepeatnumber(pchrepeatfile, number_of_core_kmers,
				number_of_outer_kmers, oligo_size, m_nStep1, core_threshold,
				outer_threshold)) {

			cloud_type3* main_oligos = new cloud_type3[number_of_core_kmers];

			readmainoligos(pchrepeatfile, main_oligos, number_of_core_kmers,
					oligo_size, m_nStep1, core_threshold);

			cout << "number of core kmers " << number_of_core_kmers << endl;

			buildmainpcloud(main_oligos, pchMainClouds, number_of_core_kmers,
					oligo_size, core_threshold);
			outputmainclouds(main_oligos, pchMainAssign, number_of_core_kmers,
					oligo_size);

			delete[] (main_oligos);

			cloud_type2* pMainCloudsA = (cloud_type2*) malloc(
					sizeof(cloud_type2) * MAXMAINMER);
			cloud_type2* pMainCloudsB = (cloud_type2*) malloc(
					sizeof(cloud_type2) * MAXMAINMER);
			cloud_type2* pMainCloudsC = (cloud_type2*) malloc(
					sizeof(cloud_type2) * MAXMAINMER);
			int numberA, numberB, numberC;

			readmainclouds(pchMainAssign, pMainCloudsA, numberA, pMainCloudsB,
					numberB, pMainCloudsC, numberC, oligo_size, m_nStep1,
					m_nStep2, m_nStep3);
			buildaccessarypcloud(pchrepeatfile, pchAccAssign, pMainCloudsA,
					numberA, pMainCloudsB, numberB, pMainCloudsC, numberC,
					number_of_outer_kmers, oligo_size, m_nStep1, core_threshold,
					outer_threshold);

			free(pMainCloudsA);
			free(pMainCloudsB);
			free(pMainCloudsC);

		} else {
			cerr << "Open word count file error." << endl;
			return (-1);
		}
	}

	if (m_nDissection) {
		bitvector PatternVector((unsigned long) pow(4, oligo_size), false);
		vector<int> CloudIdVector((unsigned long) pow(4, oligo_size), 0);

		readclouds1(pchMainAssign, pchAccAssign, PatternVector, CloudIdVector,
				oligo_size);

		if (not GenomeScanAndIdentify1(pchGenome, pchAnnotationfile,
				pchRegionfile, PatternVector, CloudIdVector, oligo_size,
				windowsize, percent, m_nChunksize, m_nGenomesize)) {
			cerr << "Genome Annotation and Repeat Region Identification Error."
					<< endl;
			PatternVector.clear();
			return (-1);
		}

		PatternVector.clear();
	}

	return (1);
}
