/*** インクルード宣言 ***/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<stdbool.h>
#include<math.h>

/*** 列挙型宣言 ***/

enum Element{FIRE=0, WATER, WIND, EARTH, LIFE, EMPTY };
enum MAX_GEMS{A, B, C, D, E, F, G, H, I, J, K, L, M, N};

/*** グローバル定数の宣言 ***/

const char ELEMENT_SYMBOLS[] = {'$', '~', '@', '#', '&', ' '};
const int ELEMENT_COLORS[] = {1, 4, 2, 3, 5, 7};
const double ELEMENT_BOOST[4][4] ={//[縦][横]. [FIRE][WATER]なら火への水攻撃
    //火、 水 、 風 、 土
    {1.0, 2.0, 0.5, 1.0},//火への攻撃
    {0.5, 1.0, 1.0, 2.0},//水への攻撃
    {2.0, 1.0, 1.0, 0.5},//風への攻撃
    {1.0, 0.5, 2.0, 1.0} //土への攻撃
};

/*** 構造体宣言 ***/

typedef struct //Monster構造体
{
    char monsterName[32];
    int type; //ここにはElementのどれかが入る。
    int maxHP;
    int HP;
    int AP;
    int DP;
} Monster;
typedef struct //ダンジョン管理
{
    Monster *monsterData; //Monster型のポインタ変数m -> 何が便利？？ポインタにすることで数に融通が利く
    int stageNum;        //stageの数
} Dungeion;              //ダンジョンに属するモンスターの一覧をまとめて管理
typedef struct //パーティー情報管理
{
    char playerName[32];
    int partyNum;
    int partyMaxHP;
    int partyHP;
    int partyDP;
    Monster *monster;
}Party;
typedef struct //BattleField構造体
{
    Party *pparty;
    Monster *pmonster;
    int Element[N+1];//要素数MAX_GEMSのElement配列？？？
}BattleField;
typedef struct //BanishInfo構造体
{
    int ElementInfo; //消える属性
    int startInfo;   //開始位置
    int serialNum;   //連続数
} BanishInfo;
typedef struct //ElementEffect
{
    int Element;
    int banishCount;
    int combo;
}ElementEffect;


/*** プロトタイプ宣言 ***/

int goDungeon(Party *currentParty, Dungeion *stage);//ダンジョン開始から終了までの流れに責任を持つ関数
int doBattle(BattleField *field);
Party organizeParty(char *playerName[], Monster *pparty, int myPartyNum);
void showParty(Monster* monsterData, int num);
void onPlayerTurn(BattleField *pfield);
void doAttack(BattleField *pfield, ElementEffect *peffect);
void onEnemyTurn(BattleField *pfield);
void doEnemyAttack(BattleField *pfield);
void showBattleField(BattleField *field);
bool checkValidCommand(char input[]);
void evaluateGems(BattleField *pfield);
bool checkBanishable(int Element[] ,BanishInfo *pbanish);
int banishGems(BattleField *pfield, BanishInfo *pbanish, int combo);
void shiftGems(int Element[], BanishInfo *pbanish, int end);
void doRecover(BattleField *pfield, int serialNum, int combo);
//void spawnGems(int Element[]);

//ユーティリティ関数
void printMonsterName(Monster *monster);//モンスターの情報を受け取り、表示に色をつける
void fillGems(int Element[]);
void printGems(int Element[]);
void printGem(int ElementNum);
void moveGem(char input[], int Element[]);
void swapGem(int start, int next, int Element[]);
int calcRecoverDamage(int serialNum, int combo);
int calcAttackDamage(BattleField *pfield, ElementEffect *peffect);
int calcEnemyAttackDamage(BattleField *pfield);
int blurDamage(double amount);

/*** 関数宣言 ***/

int main(int argc, char *argv[])/*起動と終了を管理*/
{
    //乱数生成準備
    srand((unsigned)time(0UL));

    //ゲーム開始 ー プレイヤー名の判定
    if(argc != 2){
        puts("プレイヤー名を指定して起動してください。");
        exit(1);
    }
    //タイトル
    printf("\n\x1b[7m*** Puzzle & Monsters ***\x1b[0m\n");

    //味方パーティー
    int myPartyNum = 4;
    Monster party01[] = {
        {"朱雀", FIRE, 150, 150, 25, 10},
        {"青龍", WIND, 150, 150, 15, 10},
        {"白虎", EARTH, 150, 150, 20, 5},
        {"玄武", WATER, 150, 150, 20, 15}};
    Party currentParty=organizeParty(&argv[1], party01, myPartyNum);

    //ダンジョンに出現するモンスターの一覧
    Monster data01[] = {
        {"スライム", WATER, 100, 100, 10, 5},
        {"ゴブリン", EARTH, 200, 200, 20, 10},
        {"オオコウモリ", WIND, 300, 300, 30, 25},
        {"ウェアウルフ", WIND, 400, 400, 40, 30},
        {"ドラゴン", FIRE, 800, 800, 50, 40}};
    //ステージ1のモンスター数とステージ数
    Dungeion stage1 = {data01, 5};

    //結果判定
    int gameResult;
    gameResult = goDungeon(&currentParty, &stage1);
    if (gameResult == stage1.stageNum)
        puts("\x1b[7*** GAME CLEARED ! ***\x1b[0m");
    else
        puts("\x1b[7*** GAME OVER... ***\x1b[0m");
    printf("倒したモンスター数：%d\n",gameResult);

    return 0;
}

//ダンジョンイベント
int goDungeon(Party *currentParty, Dungeion *stage)
{
    //BattleField構造体の宣言と代入
    BattleField field;
    field.pparty = currentParty;

    int i, battleCount = 0;
    printf("\n%sのパーティ(HP=%d)はダンジョンに到着した\n", field.pparty->playerName, field.pparty->partyMaxHP);

    //パーティー情報表示
    showParty(field.pparty->monster, field.pparty->partyNum);

    for (i = 0; i < stage->stageNum; i++)
    {
        field.pmonster = &stage->monsterData[i];
        if(doBattle(&field) != 0){
        //ゲーム終了
        puts("目の前が真っ暗になった...\n");
            break;//あっているか不明
        }else{
        //ゲーム継続
            battleCount++;
            //最終ゲーム
            if(battleCount == stage->stageNum){
                printf("%sはダンジョンを制覇した！\n",field.pparty->playerName);
            //ゲーム途中
            }else{
                printf("%sはさらに奥へと進んだ\n", field.pparty->playerName);
                puts("\n============================\n");
            }
        }
    }
    return battleCount;
}

//バトルイベント
int doBattle(BattleField *field)//元: Monster *monster, BattleField *field
{
    //2回目の呼び出しにはゴブリン情報の先頭アドレスが格納されている
    //field->pmonster[battleCount] = monster;
    printMonsterName(field->pmonster);//元: monster
    puts("が現れた！！！\n");

    //配列の確認: 0以上, 5以下の数値が宝石配列には格納されていなければならない
    for (int i = 0; i <= N; i++)
        if(field->Element[i]<0 || field->Element[i]>5) field->Element[i] = EMPTY;

    //宝石の生成
    fillGems(field->Element);

    //戦闘開始
    while(1){
        //プレイヤーの攻撃
        onPlayerTurn(field);
        //勝利判定
        if(field->pmonster->HP <= 0){
            printMonsterName(field->pmonster);
            puts("を倒した！");
            return 0;
        }
        //敵モンスターの攻撃
        onEnemyTurn(field);
        if(field->pparty->partyHP <= 0){
            printf("\n%sたちは敗れた\n", field->pparty->playerName);
            return 1;
        }
    }
}

//パーティー構造体管理
Party organizeParty(char *playerName[], Monster *pparty, int myPartyNum)
{
    Party currentParty;
    int i,totalHP=0, maxHP=0, totalDP=0;
    for (i = 0; i < myPartyNum; i++)
    {
        maxHP += pparty[i].maxHP;
        totalHP += pparty[i].HP;
        totalDP += pparty[i].DP;
    }
    //現在のパーティ情報を格納
    strcpy(currentParty.playerName, *playerName);
    currentParty.partyNum = myPartyNum;
    currentParty.partyMaxHP = maxHP;
    currentParty.partyHP = totalHP;
    currentParty.partyDP = totalDP/myPartyNum;//防御力は平均値を格納している
    currentParty.monster = pparty;

    return currentParty;//
}

//出陣時のパーティ構成とステータス表示
void showParty(Monster* partyData, int num)
{
    puts("\n--＜パーティ編成＞-----------");

    int i;
    for(i=0; i<num; i++)
    {
        printMonsterName(&partyData[i]);
        printf(" HP：%4d  攻撃：%3d 防御：%3d\n", partyData[i].HP, partyData[i].AP, partyData[i].DP);
    }
    puts("-----------------------------\n");
}

//プレイヤーターンの管理
void onPlayerTurn(BattleField *pfield)
{
    printf("\n【%sのターン】\n", pfield->pparty->playerName);

    //敵の名前・HP||味方の名前・HPを表示 ▶︎ 宝石表示
    showBattleField(pfield);

    //文字列の入力と判定
    char input[8];
    bool result = false;
    while(!result){
        printf("移動させる２文字を入力 ▶︎ ");
        scanf("%s",input);
        result = checkValidCommand(input);
        if(!result) puts("！入力ミスがあります！\n");
    }
    //宝石の移動
    moveGem(input, pfield->Element);

    //宝石の評価
    evaluateGems(pfield);

    return;
}
//プレイヤー攻撃の管理***********************攻撃数値強化中
void doAttack(BattleField *pfield, ElementEffect *peffect)
{
    printMonsterName(pfield->pmonster);
    puts("の攻撃！");
    //Elementからのモンスター判定からの攻撃力を。
    int giveDamageAmount = calcAttackDamage(pfield, peffect)+50;

    printf("%sに%dのダメージ！\n\n", pfield->pmonster->monsterName, giveDamageAmount);
    pfield->pmonster->HP -= giveDamageAmount;
    return;
}

//敵ターン管理
void onEnemyTurn(BattleField *pfield)
{
    printf("%sのターン\n", pfield->pmonster->monsterName);
    doEnemyAttack(pfield);
    return;
}
//敵攻撃の管理
void doEnemyAttack(BattleField *pfield)
{
    int takeDamageAmount = calcEnemyAttackDamage(pfield);
    printf("%sたちは%dのダメージを受けた。\n", pfield->pparty->playerName, takeDamageAmount);
    pfield->pparty->partyHP -=takeDamageAmount;
    return;
}

//現在のバトルフィールド表示
void showBattleField(BattleField *pfield)
{
    puts("-----------------------------\n");
    printf("        ");
    printMonsterName(pfield->pmonster);
    printf("\n        ");
    printf("HP = %4d / %4d\n", pfield->pmonster->HP, pfield->pmonster->maxHP);
    puts("\n");
    int i;
    for(i=0; i < pfield->pparty->partyNum ; i++)
    {
        printf(" ");
        printMonsterName(&pfield->pparty->monster[i]);
        printf(" ");
    }
    printf("\n       ");
    printf("HP = %4d / %4d\n", pfield->pparty->partyHP, pfield->pparty->partyMaxHP);

    puts("\n-----------------------------\n");

    //AからNを表示
    for(i=0; i<=N; i++)
        printf(" %c", 'A'+i);
    puts("");

    //printGems関数：14個の宝石表示 ▶︎ printGem関数：1個の宝石生成
    printGems(pfield->Element);
    puts("-----------------------------\n");

    return;
}

//移動コマンドが有効かどうかを判定
bool checkValidCommand(char input[])
{
    //指定した個数ではない入力を検知
    if(strlen(input)!=2) return false;
    //A~Nでないものを検知
    char input0=input[0]-'A', input1=input[1]-'A';
    if((input0<0 || input0>13)||(input1<0 || input1>13)) return false;
    //同じ文字が入力
    if(input0 == input1) return false;

    //上記をクリアした場合
    return true;
}

//宝石列を評価
void evaluateGems(BattleField *pfield)
{
    //宝石配列で消える先頭の位置を格納
    int i, end, comboCount=0;

    //消える宝石を管理する構造体
    BanishInfo info;

    //宝石消去の有無を確認
    //消去あり
    while(checkBanishable(pfield->Element ,&info))
    {
        comboCount++;
        end = banishGems(pfield, &info, comboCount);//endにはEMPTYの末尾
        //AからNを表示
        for (i = 0; i <= N; i++)
            printf(" %c", 'A' + i);
        puts("");
        shiftGems(pfield->Element, &info, end);
        fillGems(pfield->Element);

        printGems(pfield->Element);
    }
    if(comboCount == 0) puts("宝石を消せませんでした。");
    else printf("\x1b[43m%dコンボ！\x1b[49m\n",comboCount);
}

//宝石消去の有無を確認
bool checkBanishable(int Element[], BanishInfo *pbanish)
{
    int i, j, numCount=2;
    //繰り返し文: 宝石全体を管理
    for(i=0, j=i+1; i<M; i++, j++){
        if(Element[i] == Element[j]) //● ● ■
        {
            //繰り返し文: ２つ並び以上の宝石を管理
            for( ; j<=N; j++){       //● ● ●
                //3こ以上
                if(Element[i] == Element[j+1]){
                    numCount++;
                }else{               //● ● ×
                    break;
                }
            }
        }
        if (numCount > 2)
        {
            pbanish->ElementInfo = Element[i]; //格納されている数値で属性を管理している
            pbanish->startInfo = i;            //iが開始位置
            pbanish->serialNum = numCount;     //何個連続しているか

            return 1;
        }
    }
    return 0;//3つ以上続かなければ「0」を返す。
}

//宝石を消去する関数
int  banishGems(BattleField *pfield, BanishInfo *pbanish, int combo)
{
    //属性効果: stoneElement変数に属性を示す数値が格納
    ElementEffect effect = {pbanish->ElementInfo, pbanish->serialNum, combo};

    //ENPTYを格納。色を変更して表示
    int i=0;
    while (i<pbanish->serialNum)
    {
        pfield->Element[pbanish->startInfo+i] = EMPTY;
        i++;
    }
    printGems(pfield->Element);

    //命宝石か
    if(effect.Element == LIFE){
        doRecover(pfield, pbanish->serialNum, combo);
    }else{
    //それ以外の攻撃宝石か
        //属性処理
        doAttack(pfield, &effect);
    }
    return pbanish->startInfo+i-1;//「pbanish->startInfo」にはEMPTYの先頭が格納, EMPTYと次の宝石の位置を入れ替え最後尾に移動させる,serialNum回繰り返す
}

//空きスロットの右にある宝石を左に移動させる関数
void shiftGems(int Element[], BanishInfo *pbanish, int end)
{
    //shiftgems関数: 左にひとつずつ詰めていく
    //countGems関数やmoveGem関数の使用が推奨
    //countGems関数：宝石配列内に、指定された属性がいくつあるか数えるユーティリティ関数

    int i, j, temp[14];
    //ここはEMPTYを移動させる。消える宝石分だけ繰り返す
    for (i = 0; i < pbanish->serialNum; i++){

        //ここは宝石をひとつづつ左へ詰めていく機能
        for(j=0; j<14-end; j++){
            Element[end+j-i] = Element[end+j+1-i];
        }
        //末尾にEMPTYを格納
        Element[N-i] = EMPTY;

        //次回も一緒なら繰り返し終了
        if(memcmp(temp, Element, (sizeof(int))*14)==0) break;

        //ここに今回の宝石配列をコピー
        memcpy(temp, Element, (sizeof(int))*14);

        printGems(Element);
    }
    return;
}

//回復する関数
void doRecover(BattleField *pfield, int serialNum, int combo)
{
    int recoveryAmount;
    recoveryAmount = calcRecoverDamage(serialNum, combo);
    //実際の値の変動
    pfield->pparty->partyHP += recoveryAmount;
    if(pfield->pparty->partyHP > pfield->pparty->partyMaxHP)
        pfield->pparty->partyHP = pfield->pparty->partyMaxHP;
    //表示
    printf("%d回復\n", recoveryAmount);
    return;
}


/*** ユーティリティ関数 ***/

//色・記号をつける関数
void printMonsterName(Monster *monster)
{
    printf("\x1b[3%dm%c%s%c", ELEMENT_COLORS[monster->type], ELEMENT_SYMBOLS[monster->type], monster->monsterName, ELEMENT_SYMBOLS[monster->type]);
    printf("\x1b[39m");
}

//宝石を管理する配列の中に乱数を発生させる
void fillGems(int Element[])
{
    //渡された配列に14個揃っていなかったら宝石を埋める関数
    int i;
    for (i = 0; i <= N; i++){
        if(Element[i] == EMPTY){
            Element[i] = rand( ) % 5 ;
        }
    }
    return;
}

//宝石14個を表示
void printGems(int Element[])
{
    int i;
    for(i=A; i<=N; i++){
        printf(" ");
        printGem(Element[i]);
    }
    puts("\n");
    return;
}

//宝石１個を表示
void printGem(int ElementNum)
{
    printf("\x1b[4%dm\x1b[37m%c", ELEMENT_COLORS[ElementNum], ELEMENT_SYMBOLS[ElementNum]);
    printf("\x1b[49m\x1b[39m");
    return;
}

//表示された宝石を移動させる。移動の過程も表示させている。
void moveGem(char input[], int Element[])//inputはコマンドで入力された文字列配列
{
    //現在の宝石の並びを表示
    printGems(Element);

    //変化を表示
    char i, start=input[0]-'A', goal=input[1]-'A', result;
    result = (start < goal)? 1 : -1 ;
    for (i = start; i != goal; i += result)
    {
        swapGem(i, i + result, Element);
        printGems(Element);
    }
    return;
}

//これは１個だけ位置を変える関数
void swapGem(int start, int next, int Element[])
{
    int temp;
    temp = Element[next];
    Element[next] = Element[start];
    Element[start] = temp;
    return;
}

//回復量を計算する関数
int calcRecoverDamage(int serialNum, int combo)
{
    double result;
    result = 20.0 * pow(1.5, (double)serialNum-3.0 + (double)combo);

    result = (double)blurDamage(result);

    return (int)result;
}

//与えるダメージ量を計算する関数
int calcAttackDamage(BattleField *pfield, ElementEffect *peffect)
{
    //消えた宝石から、どのモンスターが攻撃するかを判定
    //Elementで判断
    int i, myMonsterAP=0;
    for(i=0; i<=pfield->pparty->partyNum ; i++){//条件式に味方モンスター数を
        if(pfield->pparty->monster[i].type == peffect->Element){
            myMonsterAP = pfield->pparty->monster[i].AP;
            //本来ならもっとパーティがいたらTypeが重複するから...
        }
    }
    //味方モンスターによる敵への攻撃ダメージ
    double result, formula1, formula2, formula3;
    formula1 = (double)(myMonsterAP - pfield->pmonster->DP);
    formula2 = ELEMENT_BOOST[pfield->pmonster->type][peffect->Element];
    formula3 = pow(1.5, (double)peffect->banishCount-3.0 + (double)peffect->combo);

    if(formula1<=0) formula1 = 1;
    result = formula1 * formula2 * formula3;

    result = (double)blurDamage(result);

    //このあたりのキャスト変換をどうとらえるか
    return (int)result;
}
//敵攻撃を計算する関数
int calcEnemyAttackDamage(BattleField *pfield)
{
    int result;
    result = (pfield->pmonster->AP - pfield->pparty->partyDP );
    result = blurDamage(result);

    if(result<=0) result = 1;
    return result;
}
//攻撃・回復量に幅を持たせる関数
int blurDamage(double amount)
{
    double min, max;
    int result;

    //0.8~1.2倍した乱数をresultに格納
    min = amount * 0.8;
    max = amount * 1.2;
    result = (int)min + (int)( rand()*(max - min + 1.0) / (1.0 + RAND_MAX) );
    return (int)result;
}
