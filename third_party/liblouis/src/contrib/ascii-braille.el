;;; ascii-braille.el --- Replace Unicode Braille with Ascii Braille overlays

;; Copyright (C) 2020 Christian Egli

;; Author: Christian Egli <christian.egli@sbs.ch>
;; Version: 0.1
;; URL: https://github.com/liblouis/ascii-braille-overlay/
;; Keywords: faces, matching, braille
;; Package-Requires: ((emacs "24.3") (ht "2.2"))

;; This program is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.

;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;;; Commentary:

;; Replace unicode braille with ascii braille overlays while. It was
;; originally inspired by the package `symbol-overlays'.

;; Usage

;; Toggle the `ascii-braille-mode' minor mode to see the unicode
;; braille in your buffer as ascii braille. Disabling the minor will
;; remove all overlays.

;; TODO

;; - I suppose it would be nice if you could edit the ascii braille
;;   and have it reflected in the unicode braille.
;; - Add a modify hook that updates the ascii braille if the unicode
;;   braille changes.


;;; Code:

(require 'ht)

(defconst ascii-braille-unicode-to-ascii-map
  (ht
   (?⠀ ? )
   (?⠁ ?a)
   (?⠃ ?b)
   (?⠉ ?c)
   (?⠙ ?d)
   (?⠑ ?e)
   (?⠋ ?f)
   (?⠛ ?g)
   (?⠓ ?h)
   (?⠊ ?i)
   (?⠚ ?j)
   (?⠅ ?k)
   (?⠇ ?l)
   (?⠍ ?m)
   (?⠝ ?n)
   (?⠕ ?o)
   (?⠏ ?p)
   (?⠟ ?q)
   (?⠗ ?r)
   (?⠎ ?s)
   (?⠞ ?t)
   (?⠥ ?u)
   (?⠧ ?v)
   (?⠺ ?w)
   (?⠭ ?x)
   (?⠽ ?y)
   (?⠵ ?z)
   (?⠬ ?0)
   (?⠡ ?1)
   (?⠣ ?2)
   (?⠩ ?3)
   (?⠹ ?4)
   (?⠱ ?5)
   (?⠫ ?6)
   (?⠻ ?7)
   (?⠳ ?8)
   (?⠪ ?9)
   (?⠯ ?&)
   (?⠿ ?%)
   (?⠷ ?{)
   (?⠮ ?~)
   (?⠾ ?})
   (?⠂ ?,)
   (?⠆ ?\;)
   (?⠒ ?:)
   (?⠲ ?/)
   (?⠢ ??)
   (?⠖ ?+)
   (?⠶ ?=)
   (?⠦ ?()
   (?⠔ ?*)
   (?⠴ ?))
   (?⠄ ?.)
   (?⠤ ?-)
   (?⠌ ?|)
   (?⠜ ?`)
   (?⠼ ?#)
   (?⠈ ?\")
   (?⠐ ?!)
   (?⠘ ?>)
   (?⠨ ?$)
   (?⠸ ?_)
   (?⠰ ?<)
   (?⠠ ?')))

(defconst ascii-braille-dots-to-unicode-map
  '((?1 #x1)
    (?2 #x2)
    (?3 #x4)
    (?4 #x8)
    (?5 #x10)
    (?6 #x20)
    (?7 #x40)
    (?8 #x80)))

(defun ascii-braille-dots-to-unicode (dots)
  "Convert a string of DOTS such as \"1234\" to a unicode character."
  ;; see https://en.wikipedia.org/wiki/Braille_Patterns
  (let* ((offsets (seq-map (lambda (tuple)
			     (seq-let (dot value) tuple
			       (if (seq-contains dots dot) value 0)))
			   ascii-braille-dots-to-unicode-map))
	 (offset-sum (seq-reduce #'+ offsets 0))
	 (unicode-braille-base #x2800))
    (+ unicode-braille-base offset-sum)))

(defgroup ascii-braille nil
  "Convert unicode braille to ascii braille using overlays."
  :group 'convenience)

(defface ascii-braille-overlay-default-face
  '((t (:inherit highlight)))
  "Ascii braille overlay default face"
  :group 'ascii-braille)

(defun ascii-braille-unicode-to-ascii (char)
  "Convert a unicode braille char to ascii braille. If the char
is not a unicode braille char just return the char itself."
  (ht-get ascii-braille-unicode-to-ascii-map char char))

(defun ascii-braille-to-ascii (s)
  "Convert a unicode braille string to a ascii braille string."
  (seq-concatenate 'string (seq-map #'ascii-braille-unicode-to-ascii s)))

(defun ascii-braille-overlay-put-one ()
  "Put overlay on current occurrence of unicode braille after a match."
  (let ((ov (make-overlay (match-beginning 0) (match-end 0)))
	(ascii-braille (ascii-braille-to-ascii (match-string 0))))
    ; (overlay-put ov 'face 'ascii-braille-overlay-default-face)
    ;(overlay-put ov 'invisible t)
    (overlay-put ov 'evaporate t)
    (overlay-put ov 'braille t)
    (add-face-text-property 0 (length ascii-braille) 'ascii-braille-overlay-default-face nil ascii-braille)
    (overlay-put ov 'before-string ascii-braille)))

(defun ascii-braille-overlay-put-all ()
  "Put overlays on all occurrences of unicode braille in the buffer."
  (let* ((case-fold-search nil))
    (save-excursion
      (save-restriction
        (goto-char (point-min))
        (let ((re "[⠀-⠿]+"))
          (while (re-search-forward re nil t)
            (ascii-braille-overlay-put-one)))))))


(define-minor-mode ascii-braille-mode
  "Toggle Ascii Braille mode.
Hide all Unicode Braille and put an overlay with Ascii Braille over top of it."
  ;; The initial value.
  nil
  ;; The indicator for the mode line.
  " AsciiBraille"
  :group 'ascii-braille
  (if ascii-braille-mode
      (ascii-braille-overlay-put-all)
    (remove-overlays nil nil 'braille t)))
