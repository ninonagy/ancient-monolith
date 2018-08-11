// engine_string.cpp

internal void
CatStrings(size_t SourceACount, char *SourceA,
           size_t SourceBCount, char *SourceB,
           size_t DestCount, char *Dest)
{
    for(int Index = 0;
        Index < SourceACount;
        ++Index)
    {
        *Dest++ = *SourceA++;
    }
    
    for(int Index = 0;
        Index < SourceBCount;
        ++Index)
    {
        *Dest++ = *SourceB++;
    }
    
    *Dest++ = 0;
}

internal int
StringLength(char *String)
{
    int Count = 0;
    while(*String++)
    {
        ++Count;
    }
    
    return Count;
}

internal b32
StringsAreEqual(char* A, char* B)
{
    while(*A && *B && *A == *B)
    {
        ++A;
        ++B;
    }

    b32 Result = (*A == 0 && *B == 0);
    return (Result);
}