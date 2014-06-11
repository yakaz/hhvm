<?hh

class BDBException extends RuntimeException { }

class BDB {
  private ?resource $__db = null;

  <<__Native>>
  public function __construct(string $path, int $type = DB_UNKNOWN,
    int $flags = 0, int $mode = 0666): void;

  <<__Native>>
  public function exists(string $key): bool;

  <<__Native>>
  public function get(string $key): mixed;

  <<__Native>>
  public function put(string $key, string $value, int $flags = 0): void;

  <<__Native>>
  public function del(string $key): void;

  <<__Native>>
  public function close(): void;
}
