/* 점수 저장/불러오기
 * 파일 한 줄은 "점수 라운드 이름" */
#include "bubble.h"

// 기록이 맞는 값인지 확인
static bool valid_score(BbScore score)
{
    if (score.round < 1 || score.round > BB_SCORE_MAX_ROUND)
        return false;
    if (score.name[3] != '\0')
        return false;
    for (int i = 0; i < 3; ++i)
    {
        char letter = score.name[i];
        bool uppercase = letter >= 'A' && letter <= 'Z';
        bool digit = letter >= '0' && letter <= '9';
        if (!uppercase && !digit && letter != '.')
            return false;
    }
    return true;
}

// 순위표에 기록 넣기 (점수 높은 순)
void bb_scores_insert(BbScore scores[BB_SCORE_COUNT], int *count, BbScore score)
{
    if (!valid_score(score))
        return;
    // 들어갈 자리 찾기
    int position = 0;
    while (position < *count &&
           (scores[position].score > score.score ||
            (scores[position].score == score.score && scores[position].round >= score.round)))
    {
        ++position;
    }
    if (position >= BB_SCORE_COUNT)
        return;
    if (*count < BB_SCORE_COUNT)
        ++*count;
    // 뒤에서부터 한 칸씩 밀기
    for (int i = *count - 1; i > position; --i)
        scores[i] = scores[i - 1];
    scores[position] = score;
}

// 파일에서 점수 읽기
bool bb_scores_load(const char *path, BbScore scores[BB_SCORE_COUNT], int *count)
{
    *count = 0;
    FILE *file = fopen(path, "r");
    if (file == NULL)
        return true; // 파일 없음
    char line[128];
    while (fgets(line, sizeof line, file) != NULL)
    {
        BbScore record = {0};
        char name[8];
        // 한 줄에서 값 꺼내기
        if (sscanf(line, "%u %d %7s", &record.score, &record.round, name) != 3)
            continue;
        if (strlen(name) != 3)
            continue;
        strcpy(record.name, name);
        bb_scores_insert(scores, count, record);
    }
    fclose(file);
    return true;
}

// 점수 파일에 쓰기
bool bb_scores_save(const char *path, const BbScore *scores, int count)
{
    FILE *file = fopen(path, "w");
    if (file == NULL)
        return false;
    for (int i = 0; i < count; ++i)
        fprintf(file, "%u %d %s\n", scores[i].score, scores[i].round, scores[i].name);
    fclose(file);
    return true;
}
