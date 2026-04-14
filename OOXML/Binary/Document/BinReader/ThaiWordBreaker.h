/*
 * ThaiWordBreaker.h
 *
 * Dictionary-based maximum-matching word segmentation for Thai text.
 * Used by BinaryReaderD to inject <w:br/> at Thai word boundaries when
 * converting binary → DOCX for paragraphs with w:jc="thaiDistribute".
 *
 * The dictionary is loaded from the path supplied to Init().
 * In the edoc-office Docker image, the file is installed at:
 *   /var/lib/onlyoffice/documentserver/dictionary/words_th.txt
 *
 * If the dictionary is not found, segmentation falls back to character-by-character
 * (no word breaks are inserted).
 */
#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace BinDocxRW
{

/**
 * Lightweight Thai word segmenter (maximum-matching, greedy).
 *
 * Usage:
 *   ThaiWordBreaker breaker;
 *   breaker.Init("/path/to/words_th.txt");
 *   std::vector<std::wstring> words = breaker.Segment(L"การทำงาน");
 *   // → {L"การ", L"ทำงาน"}
 */
class ThaiWordBreaker
{
public:
    ThaiWordBreaker();

    /**
     * Load dictionary from a plain-text file (one word per line, UTF-8).
     * Safe to call multiple times; subsequent calls replace the dictionary.
     * Returns true on success, false if file cannot be opened.
     */
    bool Init(const std::string& sDictPath);

    /**
     * Segment a string of Thai characters into words.
     * Non-Thai characters are returned as individual single-character segments.
     * Consecutive unrecognised Thai characters are each returned as individual chars.
     *
     * The returned vector lists each segment (word or single char) in order.
     */
    std::vector<std::wstring> Segment(const std::wstring& sText) const;

    /**
     * Returns true if the dictionary has been loaded (non-empty).
     */
    bool IsLoaded() const;

    /** Thai Unicode block: U+0E00 – U+0E7F */
    static bool IsThai(wchar_t c);

private:
    static const int MAX_WORD_LEN = 20;

    std::unordered_set<std::wstring> m_dict;

    /** Segment a pure-Thai sequence using maximum matching. */
    std::vector<std::wstring> SegmentThaiSequence(const std::wstring& sText,
                                                   int nStart,
                                                   int nEnd) const;
};

} // namespace BinDocxRW
