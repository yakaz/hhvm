<?hh

class LMDBException extends RuntimeException { }

class LMDBEnv {
  private ?resource $__env = null;

  <<__Native>>
  public function __construct(string $path, ?array $options = null): void;

  <<__Native>>
  public function stat(): array;

  public function open_dbi(?string $name = null, int $flags = 0): LMDBDbi {
    return new LMDBDbi($this, $name, $flags);
  }

  public function begin_txn(int $flags = 0): LMDBTxn {
    return new LMDBTxn($this, $flags);
  }

  <<__Native>>
  public function close(): void;
}

class LMDBDbi {
  private ?resource $__env = null;
  private ?resource $__dbi = null;

  <<__Native>>
  public function __construct(object $env, ?string $name, int $flags): void;

  <<__Native>>
  public function stat(object $txn): array;

  <<__Native>>
  public function close(): void;

  public function get_env(): LMDBEnv {
    return $__env;
  }
}

class LMDBTxn {
  private ?resource $__parent = null;
  private ?resource $__txn = null;

  <<__Native>>
  public function __construct(object $parent, int $flags): void;

  public function begin_txn(int $flags = 0): LMDBTxn {
    return new LMDBTxn($this, $flags);
  }

  <<__Native>>
  public function exists(object $dbi, string $key): bool;

  <<__Native>>
  public function get(object $dbi, string $key): mixed;

  <<__Native>>
  public function put(object $dbi, string $key, string $value,
    int $flags = 0): void;

  <<__Native>>
  public function del(object $dbi, string $key, ?string $value = null): void;

  <<__Native>>
  public function commit(): void;

  <<__Native>>
  public function abort(): void;

  public function get_parent(): resource {
    return $__parent;
  }
}
