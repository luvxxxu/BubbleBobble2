/* 순위표 계산과 파일 저장/읽기를 검사한다. 창이나 그림 없이 함수만 부른다.
 * assert는 조건이 거짓이면 즉시 시험을 멈춘다. */
#include "bubble.h"

#include <assert.h>

/* 점수 내림차순, 같은 점수는 높은 라운드 먼저, 완전히 같으면 먼저 있던 기록이 앞이다. */
static void test_insert_order(void)
{
    BbScore scores[BB_SCORE_COUNT] = {0};
    int count = 0;
    BbScore a = {100, 1, "AAA"}, b = {300, 2, "BBB"}, c = {300, 3, "CCC"}, d = {300, 3, "DDD"};
    bb_scores_insert(scores, &count, a);
    bb_scores_insert(scores, &count, b);
    bb_scores_insert(scores, &count, c);
    bb_scores_insert(scores, &count, d);
    assert(count == 4);
    assert(strcmp(scores[0].name, "CCC") == 0); /* 같은 점수와 라운드면 먼저 들어온 기록이 앞 */
    assert(strcmp(scores[1].name, "DDD") == 0);
    assert(strcmp(scores[2].name, "BBB") == 0);
    assert(strcmp(scores[3].name, "AAA") == 0);
}

/* 열한 번째 기록은 가장 낮은 기록을 밀어내고, 너무 낮으면 들어가지 못한다. */
static void test_limit_and_invalid(void)
{
    BbScore scores[BB_SCORE_COUNT] = {0};
    int count = 0;
    for (int i = 0; i < 12; ++i)
    {
        BbScore record = {(unsigned int)(1000 + i * 100), 1, "AAA"};
        bb_scores_insert(scores, &count, record);
    }
    assert(count == BB_SCORE_COUNT);
    assert(scores[0].score == 2100);
    assert(scores[BB_SCORE_COUNT - 1].score == 1200);
    BbScore bad_round = {5000, 9, "AAA"};
    BbScore bad_name = {5000, 1, "aaa"};
    bb_scores_insert(scores, &count, bad_round);
    bb_scores_insert(scores, &count, bad_name);
    assert(scores[0].score == 2100); /* 잘못된 기록은 무시한다. */
}

/* 저장한 파일을 다시 읽으면 같은 기록이 나오고, 파일이 없으면 빈 순위표다. */
static void test_save_and_load(const char *path)
{
    BbScore scores[BB_SCORE_COUNT] = {0};
    BbScore loaded[BB_SCORE_COUNT] = {0};
    int count = 0, loaded_count = 0;
    BbScore a = {20000, 1, "JUL"}, b = {4000, 3, "KEL"};
    bb_scores_insert(scores, &count, a);
    bb_scores_insert(scores, &count, b);
    assert(bb_scores_save(path, scores, count));
    assert(bb_scores_load(path, loaded, &loaded_count));
    assert(loaded_count == 2);
    assert(loaded[0].score == 20000 && loaded[0].round == 1 && strcmp(loaded[0].name, "JUL") == 0);
    assert(loaded[1].score == 4000 && loaded[1].round == 3 && strcmp(loaded[1].name, "KEL") == 0);
    remove(path);
    assert(bb_scores_load(path, loaded, &loaded_count) && loaded_count == 0);
}

/* 줄 모양이 틀린 기록은 건너뛰고 나머지만 읽는다. */
static void test_bad_lines(const char *path)
{
    BbScore loaded[BB_SCORE_COUNT] = {0};
    int count = 0;
    FILE *file = fopen(path, "w");
    assert(file != NULL);
    fputs("abc\n500 1 AAA\n300 9 BBB\n50 2    TOMA\n700 2 CCC\n", file);
    fclose(file);
    assert(bb_scores_load(path, loaded, &count));
    assert(count == 2);
    assert(loaded[0].score == 700 && loaded[1].score == 500);
    remove(path);
}

int main(int argc, char **argv)
{
    const char *path = "score-test-file.txt";
    if (argc > 1)
        path = argv[1];
    test_insert_order();
    test_limit_and_invalid();
    test_save_and_load(path);
    test_bad_lines(path);
    printf("score tests passed\n");
    return 0;
}
