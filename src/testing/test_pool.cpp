
#include <base/system++/pool.h>
#include <cstdlib>


const int NUM_TEST_OBJECTS = 2048;


void test_pool(int64 *pTimeStart, int num)
{
	CPool<unsigned int> Pool;

	Pool.HintSize(NUM_TEST_OBJECTS);

	*pTimeStart = time_get_raw();
	unsigned int *apArray[NUM_TEST_OBJECTS];
	for(int n = 0; n < num; n++)
	{
		for(unsigned int i = 0; i < NUM_TEST_OBJECTS; i++)
		{
			apArray[i] = Pool.Allocate(i);
			*(apArray[i]) = i;
		}

		for(unsigned int i = 0; i < NUM_TEST_OBJECTS; i++)
		{
			Pool.Free(apArray[i]);
		}
	}
}

void test_memalloc(int64 *pTimeStart, int num)
{
	*pTimeStart = time_get_raw();
	unsigned int *apArray[NUM_TEST_OBJECTS];
	for(int n = 0; n < num; n++)
	{
		for(unsigned int i = 0; i < NUM_TEST_OBJECTS; i++)
		{
			apArray[i] = (unsigned int*)mem_alloc(sizeof(unsigned int), 0);
			*(apArray[i]) = i;
		}

		for(unsigned int i = 0; i < NUM_TEST_OBJECTS; i++)
		{
			mem_free(apArray[i]);
		}
	}
}

void test_malloc(int64 *pTimeStart, int num)
{
	*pTimeStart = time_get_raw();
	unsigned int *apArray[NUM_TEST_OBJECTS];
	for(int n = 0; n < num; n++)
	{
		for(unsigned int i = 0; i < NUM_TEST_OBJECTS; i++)
		{
			apArray[i] = (unsigned int*)malloc(sizeof(unsigned int));
			*(apArray[i]) = i;
		}

		for(unsigned int i = 0; i < NUM_TEST_OBJECTS; i++)
		{
			free(apArray[i]);
		}
	}
}


#define CONDUCT_TEST(WHAT, NUM) \
	{\
		int64 start, end, dauer;\
\
		test_##WHAT(&start, NUM);\
		end = time_get_raw();\
\
		dauer = end-start;\
		double us = (double)dauer / (((double)time_freq())/1000000.0);\
		float ms = (float)dauer / (((float)time_freq())/1000.0f);\
		dbg_msg("main", #WHAT " test %i took %lli time units (%f µs = %f ms)", NUM, dauer, us, ms);\
		print_leakreport(#WHAT " " #NUM);\
	}


void print_leakreport(const char *pTestName)
{
	mem_check();

	if(mem_stats()->allocated == 0)
	{
		dbg_msg("leakreport", "all freed");
		return;
	}

	dbg_msg("leakreport", "Total of %i bytes (%d kb) not freed after test '%s' exit. Backtrace:", mem_stats()->allocated, mem_stats()->allocated>>10, pTestName);
	MEMHEADER *conductor = mem_stats()->first;
	int CurrSize = 0, CurrNum = 0;
	while(conductor)
	{
		MEMHEADER *next = conductor->next;
		CurrNum++;
		CurrSize += conductor->size;
		if(next && str_comp_nocase(conductor->filename, next->filename) == 0 && conductor->line == next->line)
		{
			conductor = next;
			continue;
		}

		dbg_msg("leakreport", "%i bytes in %i from %s:%i", CurrSize, CurrNum, conductor->filename, conductor->line);
		CurrNum = 0;
		CurrSize = 0;
		conductor = next;
	}
}

int main()
{
	dbg_logger_stdout();
	time_get_raw();

	CONDUCT_TEST(pool, 50);
	CONDUCT_TEST(memalloc, 50);
	CONDUCT_TEST(malloc, 50);
	dbg_msg("main", "------------------------");

	CONDUCT_TEST(pool, 500);
	CONDUCT_TEST(memalloc, 500);
	CONDUCT_TEST(malloc, 500);
	dbg_msg("main", "------------------------");

	CONDUCT_TEST(pool, 5000);
	CONDUCT_TEST(memalloc, 5000);
	CONDUCT_TEST(malloc, 5000);

	return 0;
}
