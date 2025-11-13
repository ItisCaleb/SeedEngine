@echo off
for /r core %%f in (*.cpp *.c *.h *.hpp *.cc) do (
    clang-format -i "%%f"
)
for /r editor %%f in (*.cpp *.c *.h *.hpp *.cc) do (
    clang-format -i "%%f"
)
for /r game %%f in (*.cpp *.c *.h *.hpp *.cc) do (
    clang-format -i "%%f"
)