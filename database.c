#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Windows.h>
#include <locale.h>

#define INITIAL_CAPACITY 2
#define MAX_NAME 100

typedef enum { ALLIANCE, HORDE } Faction;
typedef enum { TANK, HEALER, DPS } Role;

typedef union {
    struct {
        int raid_bosses_killed;
        int mythic_plus_score;
    } pve;
    struct {
        int arena_rating;
        int battleground_wins;
    } pvp;
} SpecData;

typedef struct {
    int id;
    char* nickname;
    int level;
    int achievement_points;
    int alt_count;
    Faction faction;
    Role role;
    int is_pve;
    SpecData spec;
} Player;

typedef struct {
    int* ids;
    int size;
    int capacity;
} IdSet;

typedef struct {
    Player** players;
    int size;
    int capacity;
    IdSet idset;
} Database;

void init_idset(IdSet* set) {
    set->capacity = INITIAL_CAPACITY;
    set->size = 0;
    set->ids = (int*)malloc(set->capacity * sizeof(int));
}

int id_exists(IdSet* set, int id) {
    for (int i = 0; i < set->size; i++)
        if (set->ids[i] == id)
            return 1;
    return 0;
}

void add_id(IdSet* set, int id) {
    if (id_exists(set, id)) return;
    if (set->size >= set->capacity) {
        set->capacity *= 2;
        int* temp = (int*)realloc(set->ids, set->capacity * sizeof(int));
        if (!temp) return;
        set->ids = temp;
    }
    set->ids[set->size++] = id;
}

void remove_id(IdSet* set, int id) {
    for (int i = 0; i < set->size; i++) {
        if (set->ids[i] == id) {
            for (int j = i; j < set->size - 1; j++)
                set->ids[j] = set->ids[j + 1];
            set->size--;
            return;
        }
    }
}

void free_idset(IdSet* set) { free(set->ids); }

Database* create_database() {
    Database* db = (Database*)calloc(1, sizeof(Database));
    db->capacity = INITIAL_CAPACITY;
    db->size = 0;
    db->players = (Player**)malloc(db->capacity * sizeof(Player*));
    init_idset(&db->idset);
    return db;
}

int generate_next_id(Database* db) {
    if (db->size == 0) return 1;

    int max_id = 0;
    for (int i = 0; i < db->size; i++) {
        if (db->players[i]->id > max_id) {
            max_id = db->players[i]->id;
        }
    }
    return max_id + 1;
}

Player manual_input(Database* db) {
    Player p;
    char tmp[MAX_NAME];

    p.id = generate_next_id(db);

    printf("Введите ник: "); scanf("%s", tmp);
    p.nickname = (char*)malloc(strlen(tmp) + 1);
    strcpy(p.nickname, tmp);

    printf("Уровень: "); scanf("%d", &p.level);
    printf("Achievement points: "); scanf("%d", &p.achievement_points);
    printf("Количество альтов: "); scanf("%d", &p.alt_count);
    printf("PvE (1) или PvP (0): "); scanf("%d", &p.is_pve);

    if (p.is_pve) {
        printf("Рейд-боссы убито: "); scanf("%d", &p.spec.pve.raid_bosses_killed);
        printf("Mythic+ score: "); scanf("%d", &p.spec.pve.mythic_plus_score);
    }
    else {
        printf("Arena rating: "); scanf("%d", &p.spec.pvp.arena_rating);
        printf("Battleground wins: "); scanf("%d", &p.spec.pvp.battleground_wins);
    }

    int f, r;
    printf("Фракция (0=ALLIANCE,1=HORDE): "); scanf("%d", &f);
    p.faction = (Faction)f;
    printf("Роль (0=TANK,1=HEALER,2=DPS): "); scanf("%d", &r);
    p.role = (Role)r;

    return p;
}

void insert_player(Database* db, Player player) {
    if (id_exists(&db->idset, player.id)) {
        printf("ID %d уже существует!\n", player.id);
        return;
    }

    if (db->size >= db->capacity) {
        db->capacity *= 2;
        Player** temp = (Player**)realloc(db->players, db->capacity * sizeof(Player*));
        if (!temp) return;
        db->players = temp;
    }

    Player* new_player = (Player*)malloc(sizeof(Player));
    *new_player = player;

    db->players[db->size++] = new_player;
    add_id(&db->idset, player.id);
}

Player* find_player(Database* db, int id) {
    for (int i = 0; i < db->size; i++)
        if (db->players[i]->id == id)
            return db->players[i];
    return NULL;
}

void delete_player(Database* db, int id) {
    for (int i = 0; i < db->size; i++) {
        if (db->players[i]->id == id) {
            free(db->players[i]->nickname);
            free(db->players[i]);

            db->players[i] = db->players[db->size - 1];
            db->players[db->size - 1] = NULL;
            db->size--;

            remove_id(&db->idset, id);

            if (db->size < db->capacity / 2 && db->capacity > INITIAL_CAPACITY) {
                db->capacity /= 2;
                Player** temp = (Player**)realloc(db->players,
                    db->capacity * sizeof(Player*));
                if (temp) {
                    db->players = temp;
                }
            }

            printf("Игрок удалён.\n");
            return;
        }
    }
    printf("Игрок не найден.\n");
}

void print_player(Player* p) {
    printf("ID:%d | Nick:%s | Lv:%d | Achiev:%d | Alts:%d | ",
        p->id, p->nickname, p->level, p->achievement_points, p->alt_count);

    if (p->is_pve)
        printf("PvE:Bosses %d M+ %d | ", p->spec.pve.raid_bosses_killed, p->spec.pve.mythic_plus_score);
    else
        printf("PvP:Arena %d BGWins %d | ", p->spec.pvp.arena_rating, p->spec.pvp.battleground_wins);

    printf("Faction:%s | Role:%s\n",
        p->faction == ALLIANCE ? "ALLIANCE" : "HORDE",
        p->role == TANK ? "TANK" : p->role == HEALER ? "HEALER" : "DPS");
}

void print_database(Database* db) {
    if (db->size == 0) { printf("База пуста.\n"); return; }
    for (int i = 0; i < db->size; i++)
        print_player(db->players[i]);
}

void save_to_file(Database* db, const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) { printf("Не удалось открыть файл для записи.\n"); return; }
    for (int i = 0; i < db->size; i++) {
        Player* p = db->players[i];
        fprintf(file, "%d %s %d %d %d %d %d %d %d %d\n",
            p->id, p->nickname,
            p->level, p->achievement_points, p->alt_count, p->is_pve,
            p->is_pve ? p->spec.pve.raid_bosses_killed : p->spec.pvp.arena_rating,
            p->is_pve ? p->spec.pve.mythic_plus_score : p->spec.pvp.battleground_wins,
            p->faction, p->role);
    }
    fclose(file);
    printf("Сохранено в %s\n", filename);
}

void load_from_file(Database* db, const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) { printf("Не удалось открыть файл для чтения.\n"); return; }

    int loaded = 0;
    while (!feof(file)) {
        Player p;
        char nickname[MAX_NAME];
        int n = fscanf(file, "%d %s %d %d %d %d %d %d %d %d",
            &p.id, nickname, &p.level,
            &p.achievement_points, &p.alt_count, &p.is_pve,
            &p.spec.pve.raid_bosses_killed, &p.spec.pve.mythic_plus_score,
            (int*)&p.faction, (int*)&p.role);
        if (n != 10) break;

        p.nickname = (char*)malloc(strlen(nickname) + 1);
        strcpy(p.nickname, nickname);

        if (id_exists(&db->idset, p.id)) {
            printf("Игрок с ID %d уже существует в базе\n", p.id);
            free(p.nickname);
        }
        else {
            insert_player(db, p);
            loaded++;
        }
    }
    fclose(file);
    
}

void free_database(Database* db) {
    for (int i = 0; i < db->size; i++) {
        free(db->players[i]->nickname);
        free(db->players[i]);
    }
    free(db->players);
    free_idset(&db->idset);
    free(db);
}

void search_player(Database* db) {
    int option;
    printf("Искать по:\n1. ID\n2. Никнейм\n3. Фракция\n4. Роль\n5. PvE/PvP\nВыберите: ");
    scanf("%d", &option);

    int found = 0;

    if (option == 1) {
        int id; printf("Введите ID: "); scanf("%d", &id);
        Player* p = find_player(db, id);
        if (p) { print_player(p); found = 1; }
    }
    else if (option == 2) {
        char name[MAX_NAME]; printf("Введите никнейм: "); scanf("%s", name);
        for (int i = 0; i < db->size; i++)
            if (strcmp(db->players[i]->nickname, name) == 0) { print_player(db->players[i]); found = 1; }
    }
    else if (option == 3) {
        int f; printf("Фракция (0=ALLIANCE,1=HORDE): "); scanf("%d", &f);
        for (int i = 0; i < db->size; i++)
            if (db->players[i]->faction == f) { print_player(db->players[i]); found = 1; }
    }
    else if (option == 4) {
        int r; printf("Роль (0=TANK,1=HEALER,2=DPS): "); scanf("%d", &r);
        for (int i = 0; i < db->size; i++)
            if (db->players[i]->role == r) { print_player(db->players[i]); found = 1; }
    }
    else if (option == 5) {
        int pve; printf("PvE (1) или PvP (0): "); scanf("%d", &pve);
        for (int i = 0; i < db->size; i++)
            if (db->players[i]->is_pve == pve) { print_player(db->players[i]); found = 1; }
    }

    if (!found) printf("Игроки не найдены.\n");
}

int main() {
    setlocale(LC_ALL, " ");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);
    Database* db = create_database();
    int choice;

    do {
        printf("1. Добавить игрока вручную\n");
        printf("2. Удалить игрока по ID\n");
        printf("3. Найти игрока\n");
        printf("4. Вывести базу\n");
        printf("5. Загрузить из файла input.txt\n");
        printf("6. Сохранить в файл output.txt\n");
        printf("0. Выход\n");
        printf("Выберите: ");
        scanf("%d", &choice);

        switch (choice) {
        case 1: { Player p = manual_input(db); insert_player(db, p); break; }
        case 2: { int id; printf("ID для удаления: "); scanf("%d", &id); delete_player(db, id); break; }
        case 3: search_player(db); break;
        case 4: print_database(db); break;
        case 5: load_from_file(db, "input.txt"); break;
        case 6: save_to_file(db, "output.txt"); break;
        }

    } while (choice != 0);

    free_database(db);
    return 0;
}
