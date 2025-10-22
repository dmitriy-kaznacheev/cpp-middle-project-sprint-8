# Шаг 7. Применение инструментов анализа
Оценка эффективности рефакторинга, который был проведен, 
с помощью инструментов профилирования и динамического анализа 
*Clang LibTooling*. Это исследовательская часть, в которой были 
использованы два примера:
* **perf_example.cpp**: для профилирования производительности 
* **leak_example.cpp**: для анализа утечек памяти

## Анализ утечек памяти
В файле **leak_example.cpp** базовый класс с наследником 
имеет невиртуальный деструктор, что приводит к утечке памяти при 
полиморфном удалении (ресурсы наследника не освобождаются). 
После добавления *virtual* утечка устраняется.

### До рефакторинга:
```bash
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ sed -i 's/virtual ~Base()/~Base()/g' ../tests/tests_data/leak_example.cpp 
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ g++ -g -fsanitize=address ../tests/tests_data/leak_example.cpp -o leak_before
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ ./leak_before 
=================================================================
==71549==ERROR: AddressSanitizer: new-delete-type-mismatch on 0x6e15567e0010 in thread T0:
  object passed to delete has wrong type:
  size of the allocated type:   8 bytes;
  size of the deallocated type: 1 bytes.
    #0 0x71f557e3b3fb in operator delete(void*, unsigned long) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:155
    #1 0x562131dd428c in main ../tests/tests_data/leak_example.cpp:18
    #2 0x71f557771577 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #3 0x71f55777163a in __libc_start_main_impl ../csu/libc-start.c:360
    #4 0x562131dd4164 in _start (/workspaces/cpp-middle-project-sprint-8/build/leak_before+0x1164) (BuildId: c185600b404b962d72c1c7ee03d7996047b25dca)

0x6e15567e0010 is located 0 bytes inside of 8-byte region [0x6e15567e0010,0x6e15567e0018)
allocated by thread T0 here:
    #0 0x71f557e3a2db in operator new(unsigned long) ../../../../src/libsanitizer/asan/asan_new_delete.cpp:86
    #1 0x562131dd4243 in main ../tests/tests_data/leak_example.cpp:17
    #2 0x71f557771577 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #3 0x71f55777163a in __libc_start_main_impl ../csu/libc-start.c:360
    #4 0x562131dd4164 in _start (/workspaces/cpp-middle-project-sprint-8/build/leak_before+0x1164) (BuildId: c185600b404b962d72c1c7ee03d7996047b25dca)

SUMMARY: AddressSanitizer: new-delete-type-mismatch ../tests/tests_data/leak_example.cpp:18 in main
==71549==HINT: if you don't care about these errors you may set ASAN_OPTIONS=new_delete_type_mismatch=0
==71549==ABORTING
```

### После рефакторинга:
```bash
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ ./refactor_tool ../tests/tests_data/leak_example.cpp
/workspaces/cpp-middle-project-sprint-8/build/../tests/tests_data/leak_example.cpp:5:5: warning: non-virtual destructor in the base class; added 'virtual'
    5 |     ~Base() {}  // Невиртуальный
      |     ^
1 warning generated.
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ g++ -g -fsanitize=address ../tests/tests_data/leak_example.cpp -o leak_after 
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ ./leak_after 
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ 
```

## Анализ производительности
В файле **perf_example.cpp** есть ненужное копирование в *range-for* (без &), 
что приводит к низкой производительности для больших контейнеров. 
После добавления '&' копирование устранится, и *perf* покажет улучшение 
(меньше CPU времени на копирование).

### До рефакторинга:
```bash
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ g++ -O2 -g ../tests/tests_data/perf_example.cpp -o perf_before
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ sudo perf record -g -F 99 -- ./perf_before
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0,045 MB perf.data (181 samples) ]

dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ sudo perf report
...
+   66,63%     0,00%  perf_before  perf_before        [.] _start                                                                       ▒
+   66,63%     0,00%  perf_before  libc.so.6          [.] __libc_start_main@@GLIBC_2.34                                                ▒
-   65,27%    19,48%  perf_before  libc.so.6          [.] __memset_avx2_unaligned_erms                                                 ▒
   + 45,79% __memset_avx2_unaligned_erms                                                                                               ▒
   + 19,48% _start                                                                                                                     ▒
+   60,01%     0,68%  perf_before  [kernel.kallsyms]  [k] asm_exc_page_fault                                                           ▒
+   41,15%     0,00%  perf_before  [kernel.kallsyms]  [k] exc_page_fault                                                               ▒
+   41,15%     0,00%  perf_before  [kernel.kallsyms]  [k] do_user_addr_fault                                                           ◆
+   40,23%     0,68%  perf_before  [kernel.kallsyms]  [k] handle_mm_fault                                                              ▒
+   37,80%     1,35%  perf_before  [kernel.kallsyms]  [k] __handle_mm_fault                                                            ▒
+   34,46%     0,00%  perf_before  [kernel.kallsyms]  [k] handle_pte_fault                                                             ▒
+   33,37%     0,00%  perf_before  [unknown]          [.] 0000000000000000                                                             ▒
+   32,42%     2,16%  perf_before  [kernel.kallsyms]  [k] do_anonymous_page                                                            ▒
+   22,15%    16,08%  perf_before  perf_before        [.] main                                                                         ▒
-   12,58%    12,58%  perf_before  perf_before        [.] std::vector<HeavyObject, std::allocator<HeavyObject> >::~vector()            ▒
     0                                                                                                                                 ▒
     0                                                                                                                                 ▒
     std::vector<HeavyObject, std::allocator<HeavyObject> >::~vector()                                                                 ▒
+    9,41%     1,35%  perf_before  [kernel.kallsyms]  [k] __mem_cgroup_charge                                                          ▒
+    6,26%     1,35%  perf_before  [kernel.kallsyms]  [k] folio_add_lru                                                                ▒
+    6,26%     0,00%  perf_before  [kernel.kallsyms]  [k] folio_add_lru_vma                                                            ▒
+    6,07%     0,00%  perf_before  libc.so.6          [.] __munmap  
...
```

### После рефакторинга:
```bash
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ ./refactor_tool ../tests/tests_data/perf_example.cpp
/workspaces/cpp-middle-project-sprint-8/build/../tests/tests_data/perf_example.cpp:11:21: warning: unused variable 'data' [-Wunused-variable]
   11 |         const auto& data = obj.data[0];
      |                     ^~~~
/workspaces/cpp-middle-project-sprint-8/build/../tests/tests_data/perf_example.cpp:10:21: warning: loop variable 'obj' creates a copy from type 'HeavyObject const' [-Wrange-loop-construct]
   10 |     for (const auto obj : vec) {  // Копирование без &
      |                     ^
/workspaces/cpp-middle-project-sprint-8/build/../tests/tests_data/perf_example.cpp:10:10: note: use reference type 'HeavyObject const &' to prevent copying
   10 |     for (const auto obj : vec) {  // Копирование без &
      |          ^~~~~~~~~~~~~~~~
      |                     &
/workspaces/cpp-middle-project-sprint-8/build/../tests/tests_data/perf_example.cpp:10:21: warning: range-based for loop uses copying; added '&'
   10 |     for (const auto obj : vec) {  // Копирование без &
      |                     ^
3 warnings generated.
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ g++ -O2 -g ../tests/tests_data/perf_example.cpp -o perf_after
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ sudo perf record -g -F 99 -- ./perf_after 
[ perf record: Woken up 1 times to write data ]
[ perf record: Captured and wrote 0,039 MB perf.data (143 samples) ]

dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$ sudo perf report
...
+   71,96%    15,17%  perf_after  libc.so.6          [.] __memset_avx2_unaligned_erms                                                  ▒
+   64,21%     0,00%  perf_after  [kernel.kallsyms]  [k] asm_exc_page_fault                                                            ▒
+   49,04%     0,00%  perf_after  [kernel.kallsyms]  [k] exc_page_fault                                                                ▒
+   49,04%     0,79%  perf_after  [kernel.kallsyms]  [k] do_user_addr_fault                                                            ▒
+   44,29%     1,59%  perf_after  [kernel.kallsyms]  [k] handle_mm_fault                                                               ▒
+   42,70%     0,00%  perf_after  [kernel.kallsyms]  [k] __handle_mm_fault                                                             ▒
+   41,12%     0,00%  perf_after  [kernel.kallsyms]  [k] handle_pte_fault                                                              ▒
+   41,11%     3,06%  perf_after  [kernel.kallsyms]  [k] do_anonymous_page                                                             ▒
-   28,04%    20,46%  perf_after  perf_after         [.] main                                                                          ▒
   + 14,53% 0                                                                                                                          ▒
   - 13,51% main                                                                                                                       ▒
      + __munmap                                                                                                                       ▒
+   22,11%     0,00%  perf_after  [unknown]          [.] 0000000000000000                                                              ◆
+   10,40%     0,80%  perf_after  [kernel.kallsyms]  [k] alloc_pages_mpol                                                              ▒
+   10,40%     0,00%  perf_after  [kernel.kallsyms]  [k] alloc_anon_folio                                                              ▒
+   10,40%     0,00%  perf_after  [kernel.kallsyms]  [k] vma_alloc_folio   
...
dev@6f971b75ffb5:/workspaces/cpp-middle-project-sprint-8/build$
```
